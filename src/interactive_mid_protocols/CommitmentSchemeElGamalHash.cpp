#include "../../include/interactive_mid_protocols/CommitmentSchemeElGamalHash.hpp"

CmtElGamalHashCommitter::CmtElGamalHashCommitter(shared_ptr<CommParty> channel, shared_ptr<DlogGroup> dlog, shared_ptr<CryptographicHash> hash)
				: CmtElGamalCommitterCore(channel, dlog, make_shared<ElGamalOnByteArrayEnc>(dlog, make_shared<HKDF>(new OpenSSLHMAC()))) {
	
	//During the construction of this object, the Public Key with which we set the El Gamal object gets sent to the receiver.
	if (hash->getHashedMsgSize() > bytesCount(dlog->getOrder())) {
		throw invalid_argument("The size in bytes of the resulting hash is bigger than the size in bytes of the order of the DlogGroup.");
	}
	this->hash = hash;
}

/**
* Runs COMMIT_ElGamal to commit to value H(x).
* @return the created commitment.
*/
shared_ptr<CmtCCommitmentMsg> CmtElGamalHashCommitter::generateCommitmentMsg(shared_ptr<CmtCommitValue> input, long id){

	vector<byte> hashValArray = getHashOfX(input, id);

	//After the input has been manipulated with the Hash call the super's commit function. Since the super has been initialized with ScElGamalOnByteArray
	//it will know how to take care of the byte array input.
	auto output = CmtElGamalCommitterCore::generateCommitmentMsg(make_shared<CmtByteArrayCommitValue>(make_shared<vector<byte>>(hashValArray)), id);
	auto values = commitmentMap[id];
	commitmentMap[id] = make_shared<CmtElGamalCommitmentPhaseValues>(values->getR(), input, static_pointer_cast<AsymmetricCiphertext>(values->getComputedCommitment()));
	return output;
}

/**
* Returns H(x).
* @param input should be an instance of CmtByteArrayCommitValue.
* @param id
* @return the result of the hash function of the given input.
*/
vector<byte> CmtElGamalHashCommitter::getHashOfX(shared_ptr<CmtCommitValue> input, long id) {
	//Check that the input x is in the end a byte[]
	auto in = dynamic_pointer_cast<CmtByteArrayCommitValue>(input);
	if (in == NULL)
		throw invalid_argument("The input must be of type CmtByteArrayCommitValue");
	//Hash the input x with the hash function
	auto x = *in->getXVector();
	//Keep the original commit value x and its id in the commitmentMap, needed for later (during the decommit phase).
	//hashCommitmentMap.put(Long.valueOf(id), x);

	//calculate H(x) = Hash(x)
	vector<byte> hashValArray;
	hash->update(x, 0, x.size());
	hash->hashFinal(hashValArray, 0);
	return hashValArray;
}

shared_ptr<CmtCDecommitmentMessage> CmtElGamalHashCommitter::generateDecommitmentMsg(long id) {

	//Fetch the commitment according to the requested ID
	auto x = commitmentMap[id]->getX();
	//Get the relevant random value used in the commitment phase
	auto r = commitmentMap[id]->getR();

	return make_shared<CmtElGamalDecommitmentMessage>(x->toString(), dynamic_pointer_cast<BigIntegerRandomValue>(r));
}

/**
* This function samples random commit value and returns it.
* @return the sampled commit value
*/
shared_ptr<CmtCommitValue> CmtElGamalHashCommitter::sampleRandomCommitValue()  {
	vector<byte> val;
	gen_random_bytes_vector(val, 32, random);
	return make_shared<CmtByteArrayCommitValue>(make_shared<vector<byte>>(val));
}

/**
* This function converts the given commit value to a byte array.
* @param value
* @return the generated bytes.
*/
vector<byte> CmtElGamalHashCommitter::generateBytesFromCommitValue(CmtCommitValue* value) {
	auto val = dynamic_cast<CmtByteArrayCommitValue*>(value);
	if (val == NULL)
		throw invalid_argument("The given value must be of type CmtByteArrayCommitValue");
	return *static_pointer_cast<vector<byte>>(val->getX());
}

/**
* This constructor receives as arguments an instance of a Dlog Group and an instance
* of a Cryptographic Hash such that they keep the condition that the size in bytes
* of the resulting hash is less than the size in bytes of the order of the DlogGroup.
* Otherwise, it throws IllegalArgumentException.
* An established channel has to be provided by the user of the class.
* @param channel
* @param dlog
* @param hash
*/
CmtElGamalHashReceiver::CmtElGamalHashReceiver(shared_ptr<CommParty> channel, shared_ptr<DlogGroup> dlog, shared_ptr<CryptographicHash> hash) 
					: CmtElGamalReceiverCore(channel, dlog, make_shared<ElGamalOnByteArrayEnc>(dlog, make_shared<HKDF>(new OpenSSLHMAC()))) {
		
	if (hash->getHashedMsgSize() > bytesCount(dlog->getOrder())) {
		throw invalid_argument("The size in bytes of the resulting hash is bigger than the size in bytes of the order of the DlogGroup.");
	}
	this->hash = hash;
}

/**
* Verifies that the commitment was to H(x).
*/
shared_ptr<CmtCommitValue> CmtElGamalHashReceiver::verifyDecommitment(CmtCCommitmentMsg* commitmentMsg,
	CmtCDecommitmentMessage* decommitmentMsg) {
	auto decommitment = dynamic_cast<CmtElGamalDecommitmentMessage*>(decommitmentMsg);
	auto com = dynamic_cast<CmtElGamalCommitmentMessage*>(commitmentMsg);
	if (decommitment == NULL) {
		throw invalid_argument("decommitmentMsg should be an instance of CmtElGamalDecommitmentMessage");
	}
	if (com == NULL) {
		throw invalid_argument("commitmentMsg should be an instance of CmtElGamalCommitmentMessage");
	}
	

	//Hash the input x with the hash function
	vector<byte> x;
	const string tmp = decommitment->getX();
	x.assign(tmp.begin(), tmp.end());

	//calculate H(x) = Hash(x)
	vector<byte> hashValArray;
	hash->update(x, 0, x.size());
	hash->hashFinal(hashValArray, 0);

	//Fetch received commitment according to ID
	biginteger r = dynamic_pointer_cast<BigIntegerRandomValue>(decommitment->getR())->getR();
	auto c = elGamal->encrypt(make_shared<ByteArrayPlaintext>(hashValArray), r);

	auto commitment = static_pointer_cast<ElGamalOnByteArraySendableData>(com->getCommitment());
	auto receivedCommitmentCipher = elGamal->reconstructCiphertext(commitment.get());

	if (*receivedCommitmentCipher == *c)
		//The decommitment was accepted by El Gamal core. Now, El Gamal Hash has to return the original value before the hashing.
		return make_shared<CmtByteArrayCommitValue>(make_shared<vector<byte>>(x));
	return NULL;
}

/**
* This function converts the given commit value to a byte array.
* @param value
* @return the generated bytes.
*/
vector<byte> CmtElGamalHashReceiver::generateBytesFromCommitValue(CmtCommitValue* value) {
	auto val = dynamic_cast<CmtByteArrayCommitValue*>(value);
	if (val == NULL)
		throw invalid_argument("The given value must be of type CmtByteArrayCommitValue");
	return *static_pointer_cast<vector<byte>>(value->getX());
}

shared_ptr<CmtElGamalCommitmentMessage> CmtElGamalHashReceiver::getCommitmentMsg() {

	auto elementSendable1 = dlog->getGenerator()->generateSendableData();
	vector<byte> empty;
	return make_shared<CmtElGamalCommitmentMessage>(make_shared<ElGamalOnByteArraySendableData>(elementSendable1, empty));
}