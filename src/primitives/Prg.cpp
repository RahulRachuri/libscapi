/**
* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
* 
* Copyright (c) 2016 LIBSCAPI (http://crypto.biu.ac.il/SCAPI)
* This file is part of the SCAPI project.
* DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
* and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
* FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
* 
* We request that any publication and/or code referring to and/or based on SCAPI contain an appropriate citation to SCAPI, including a reference to
* http://crypto.biu.ac.il/SCAPI.
* 
* Libscapi uses several open source libraries. Please see these projects for any further licensing issues.
* For more information , See https://github.com/cryptobiu/libscapi/blob/master/LICENSE.MD
*
* %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
* 
*/


#include "../../include/primitives/Prg.hpp"

void ScPrgFromPrf::setKey(SecretKey & secretKey) {
	prf->setKey(secretKey); //Sets the key to the underlying prf.
	
	ctr = vector<byte>(prf->getBlockSize());
	// Initializes the counter to 1.
	ctr[ctr.size() - 1] = 1;
	this->_isKeySet = true;
}

void ScPrgFromPrf::getPRGBytes(vector<byte> & outBytes, int outOffset, int outLen) {
	if (!isKeySet())
		throw IllegalStateException("secret key isn't set");

	int numGeneratedBytes = 0;	// Number of current generated bytes.
	vector<byte> generatedBytes = vector<byte>(ctr.size());

	while (numGeneratedBytes < outLen) {
		try {
			// If the prf can output any length (for example, IteratedPrfVarying) call the computeBlock with the outputLen.
			prf->computeBlock(ctr, 0, ctr.size(), outBytes, outOffset + numGeneratedBytes, outLen);
			numGeneratedBytes += outLen;
		}
		catch (out_of_range& e) {
			try {
				// if the prf can receive any input length (for example, Hmac) call the computeBlock with the ctr length.
				// the output is written to a new array because there is no guarantee that output array is long enough to hold the next output block.
				prf->computeBlock(ctr, 0, ctr.size(), generatedBytes, 0);
				//Copy the right number of generated bytes.
				if (numGeneratedBytes + (int) generatedBytes.size() <= outLen)
					outBytes.insert(outBytes.begin() + outOffset + numGeneratedBytes, generatedBytes.begin(), generatedBytes.end());
				else
					outBytes.insert(outBytes.begin() + outOffset + numGeneratedBytes, generatedBytes.begin(), generatedBytes.begin() + (outLen - numGeneratedBytes));

				// increases the number of generated bytes.
				numGeneratedBytes += ctr.size();
			}
			catch (out_of_range& e1) {
				try {
					// if the prf can receive fixed input length (for example, AES) call the computeBlock without the input length.
					// the output is written to a new array because there is no guarantee that output array is long enough to hold the next output block.
					prf->computeBlock(ctr, 0, generatedBytes, 0);
					// copy the right number of generated bytes.
					if (numGeneratedBytes + (int) generatedBytes.size() <= outLen) 
						outBytes.insert(outBytes.begin() + outOffset + numGeneratedBytes, generatedBytes.begin(), generatedBytes.end());
					else 
						outBytes.insert(outBytes.begin() + outOffset + numGeneratedBytes, generatedBytes.begin(), generatedBytes.begin()+(outLen - numGeneratedBytes)); 
					// increases the number of generated bytes.
					numGeneratedBytes += ctr.size();
				}
				catch (out_of_range& e3) {
					cerr << e3.what() << endl;
					return;
				}
			}
		}
		// Increase the counter.
		increaseCtr();
	}
}

void ScPrgFromPrf::increaseCtr() {
	//Increase the counter by one.
	int carry = 1;
	int len = ctr.size();
	for (int i = len - 1; i >= 0; i--)
	{
		int x = (ctr[i] & 0xff) + carry;
		if (x > 0xff)
			carry = 1;
		else
			carry = 0;
		ctr[i] = (byte)x;
	}
}

PrgFromOpenSSLAES::PrgFromOpenSSLAES(int cachedSize, bool isStrict) : cachedSize(cachedSize), isStrict(isStrict) {


	//allocate memory for the plaintext which is an array of indices and for the ciphertext which is the output
	//of the encryption
	//cipherChunk = (block *)_aligned_alloc(16, sizeof(block) * cachedSize);
	//posix_memalign((void**) cipherChunk, 16, sizeof(block) * cachedSize);
	//indexPlaintext = (block *)_aligned_alloc(16, sizeof(block) * cachedSize);
	//posix_memalign((void**) indexPlaintext, 16, sizeof(block) * cachedSize);
	cipherChunk = (block *)malloc(sizeof(block) * cachedSize);
	indexPlaintext = (block *)malloc(sizeof(block) * cachedSize);

	//assin zero to the array of indices which are set as the plaintext. Note that we only use the list sagnificant long part of each 128 bit.
//	memset(indexPlaintext, 0, sizeof(block) * cachedSize);


	long *plaintextArray = (long *)indexPlaintext;

	//go over the array and set the 64 list sagnificat bits for evey 128 bit value, we use only half of the 128 bit variables
	for (long i = 0; i < cachedSize; i++) {
		plaintextArray[i * 2 + 1] = i;
        plaintextArray[i * 2] = 0;
	}

}

PrgFromOpenSSLAES::PrgFromOpenSSLAES(PrgFromOpenSSLAES && old) :
	cachedSize(old.cachedSize), idxForBytes(old.idxForBytes), startingIndex(old.startingIndex), aes(old.aes),
	_isKeySet(old._isKeySet), cipherChunk(old.cipherChunk), indexPlaintext(old.indexPlaintext), isStrict(old.isStrict)
{
	old.cipherChunk = nullptr;
	old.indexPlaintext = nullptr;
}

PrgFromOpenSSLAES & PrgFromOpenSSLAES::operator=(PrgFromOpenSSLAES && other)
{

	//copy values
	cachedSize = other.cachedSize;
	idxForBytes = other.idxForBytes;
	aes = move(other.aes);
	startingIndex = other.startingIndex;
	_isKeySet = other._isKeySet;
	cipherChunk = other.cipherChunk;
	indexPlaintext = other.indexPlaintext;
	isStrict = other.isStrict;
	

	//set other values to null
	other.cipherChunk = nullptr;
	other.indexPlaintext = nullptr;

	return *this;
}

PrgFromOpenSSLAES::~PrgFromOpenSSLAES() {
	//free allocated aligned memory
	free(cipherChunk);
	free(indexPlaintext);

	//free aes
	if(aes != nullptr) {
        EVP_CIPHER_CTX_cleanup(aes);
        //delete aes;
    }
}

SecretKey PrgFromOpenSSLAES::generateKey(int keySize) {

	//NOTE----A temp way to produce a secret key. Will be changed later
	byte * buf = new byte[keySize / 8];
	if (!RAND_bytes(buf, keySize / 8))
		throw runtime_error("key generation failed");
	vector<byte> vec;
	//copy the random bytes to a vector held in the secret key
	copy_byte_array_to_byte_vector(buf, keySize / 8, vec, 0);
	SecretKey sk(vec, getAlgorithmName());
	//free the dynamic buffer
	delete[] buf;
	return sk;
}

void PrgFromOpenSSLAES::setKey(SecretKey & secretKey) {

	if (_isKeySet == false) {

		aes = new EVP_CIPHER_CTX();

		//init the aes prp using openssl
		EVP_CIPHER_CTX_init(aes);
		EVP_EncryptInit(aes, EVP_aes_128_ecb(), &(secretKey.getEncoded()).at(0), (byte *)&iv);

		//an int for to get the actual size that was encrypted. This is not used
		int outLength;
		//encrypt the array of indices that was created in the constructor
		EVP_EncryptUpdate(aes, (byte *)cipherChunk, &outLength, (byte *)indexPlaintext, cachedSize*16);

		_isKeySet = true;

	}
	else {
		//create a new aes and init it with the new secret key
		EVP_CIPHER_CTX_cleanup(aes);
		EVP_CIPHER_CTX_init(aes);
		EVP_EncryptInit(aes, EVP_aes_128_ecb(), &(secretKey.getEncoded()).at(0), (byte *)&iv);
		idxForBytes = 0; //makes sure the indices are starting from 0
		prepare();
	}

}



void PrgFromOpenSSLAES::getPRGBytes(vector<byte> & outBytes, int outOffset, int outLen) {

	//key must be set in order to get randoms
	if (!isKeySet())
		throw IllegalStateException("secret key isn't set");

	//the required number of random bytes exceeds the avaliable randoms, prepare new randoms
	if (outLen + idxForBytes > cachedSize * 16) {
		prepare();
	}

	byte* cipherInBytes = (byte*)cipherChunk;

	//Copy the output bytes to the given output array.
	copy_byte_array_to_byte_vector(&cipherInBytes[idxForBytes], outLen, outBytes, outOffset);

	//increment the byte counter 
	idxForBytes += outLen;
}

void PrgFromOpenSSLAES::prepare() {

	if (isStrict == true)
		throw overflow_error("No randoms left for a strict class");
	//set the starting point of the index. We want the ceiling of the division by 16
	startingIndex = (idxForBytes + 16 - 1) / 16;
	long *plaintextArray = (long *)indexPlaintext;
	//go over the array and set the long for evey other long, we use only half of the 128 bit variables
	for (long i = 0; i < cachedSize; i++) {
		plaintextArray[i * 2 + 1] = startingIndex + i;
	}

	int outLength;
	//encrypt the array of indices that was created in the constructor
	EVP_EncryptUpdate(aes, (byte *)cipherChunk, &outLength, (byte *)indexPlaintext, cachedSize*16);

	idxForBytes = 0;
}




void OpenSSLRC4::setKey(SecretKey & secretKey) {
	vector<byte> encodedKey = secretKey.getEncoded();
	RC4_set_key(rc4, encodedKey.size(), &encodedKey[0]); 	// set the key to the openssl object.
	_isKeySet = true; // marks this object as initialized.
	vector<byte> out(128); // RC4 has a problem in the first 1024 bits. by ignoring these bytes, we bypass this problem.
	getPRGBytes(out, 0, 128);
}



SecretKey OpenSSLRC4::generateKey(int keySize) {
	// generate a random string of bits of length keySize, which has to be greater that zero. 
	// if the key size is zero or less - throw exception
	if (keySize <= 0)
		throw invalid_argument("key size must be greater than 0");

	// the key size has to be a multiple of 8 so that we can obtain an array of random bytes which we use
	// to create the SecretKey.
	if ((keySize % 8) != 0) 
		throw invalid_argument("Wrong key size: must be a multiple of 8");
	
	vector<byte> genBytes(keySize / 8);
	if (!RAND_bytes(&genBytes[0], keySize / 8))
		throw runtime_error("key generation failed");

	// creates a secretKey from the generated bytes.
	SecretKey generatedKey(genBytes, "");
	return generatedKey;
}

void OpenSSLRC4::getPRGBytes(vector<byte> & outBytes, int outOffset, int outLen) {
	if (!isKeySet())
		throw IllegalStateException("secret key isn't set");
	
	// create an input array full with zeros. (It will be xored with the pseudo random bytes in order to get the generated bytes).
	unsigned char* in = (unsigned char*)calloc(outLen, sizeof(char));

	// prepare the output array.
	unsigned char* output = new unsigned char[outLen];

	// generate the pseudo random bytes.
	::RC4(rc4, outLen, in, output);

	//Copy the output bytes to the given output array.
	copy_byte_array_to_byte_vector(output, outLen, outBytes, 0);

	// release the allocated memory.
	free(in);
	delete[] output;
}

OpenSSLRC4::~OpenSSLRC4() {
	delete rc4;
}

shared_ptr<PrgFromOpenSSLAES> PrgSingleton::prg = nullptr;
