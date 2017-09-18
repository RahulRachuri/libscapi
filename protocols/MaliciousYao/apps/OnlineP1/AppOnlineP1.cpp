#include <boost/thread/thread.hpp>
#include <lib/include/common/CommonMaliciousYao.hpp>
#include <lib/include/primitives/CommunicationConfig.hpp>
#include <lib/include/primitives/CryptoPrimitives.hpp>
#include <libscapi/include/circuits/GarbledCircuitFactory.hpp>
#include <lib/include/primitives/CheatingRecoveryCircuitCreator.hpp>
#include <lib/include/primitives/CircuitInput.hpp>
#include <lib/include/primitives/ExecutionParameters.hpp>
#include <lib/include/OfflineOnline/primitives/BucketBundleList.hpp>
#include <lib/include/common/LogTimer.hpp>
#include <lib/include/OfflineOnline/specs/OnlineProtocolP1.hpp>

using namespace std;

//party number
const int PARTY = 1;

const string HOME_DIR = "../..";

//const string CIRCUIT_FILENAME = HOME_DIR + "/assets/circuits/AES/NigelAes.txt";
//const string CIRCUIT_INPUT_FILENAME = HOME_DIR + "/assets/circuits/AES/AESPartyOneInputs.txt";
const string COMM_CONFIG_FILENAME = HOME_DIR + string("/lib/assets/conf/PartiesConfig.txt");

//const string CIRCUIT_CHEATING_RECOVERY = HOME_DIR + "/assets/circuits/CheatingRecovery/UnlockP1Input.txt";
//const string BUCKETS_PREFIX_MAIN = HOME_DIR + "/data/P1/aes";
//const string BUCKETS_PREFIX_CR = HOME_DIR + "/data/P1/cr";

//file for dlog
const string NISTEC_FILE_NAME = "../../../../include/configFiles/NISTEC.txt";

int BUCKET_ID = 0;

int main(int argc, char* argv[]) {

//	boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));

    int counter = 1;

    auto CIRCUIT_FILENAME = HOME_DIR + argv[counter++];
    auto CIRCUIT_INPUT_FILENAME = HOME_DIR + argv[counter++];
    auto CIRCUIT_CHEATING_RECOVERY = HOME_DIR + argv[counter++];
    auto BUCKETS_PREFIX_MAIN = HOME_DIR + argv[counter++];
    auto BUCKETS_PREFIX_CR = HOME_DIR + argv[counter++];

    int N1 = atoi(argv[counter++]);
    int B1 = atoi(argv[counter++]);
    int s1 = atoi(argv[counter++]);
    double p1 = stod(argv[counter++]);
    int N2 = atoi(argv[counter++]);
    int B2 = atoi(argv[counter++]);
    int s2 = atoi(argv[counter++]);
    double p2 = stod(argv[counter++]);

	//set crypto primitives
	CryptoPrimitives::setCryptoPrimitives(NISTEC_FILE_NAME);
	CryptoPrimitives::setNumOfThreads(8);


/*	int N1 = 32;
	int B1 = 7;
	int s1 = 40;
	double p1 = 0.62;

	int N2 = 32;
	int B2 = 20;
	int s2 = 40;
	double p2 = 0.71;*/

	/*int N1 = 128;
	int B1 = 6;
	int s1 = 40;
	double p1 = 0.77;

	int N2 = 128;
	int B2 = 14;
	int s2 = 40;
	double p2 = 0.76;*/

	/*int N1 = 1024;
	int B1 = 4;
	int s1 = 40;
	double p1 = 0.72;
				
	int N2 = 1024;
	int B2 = 10;
	int s2 = 40;
	double p2 = 0.85;*/
	
	// we load the bundles from file
	vector<shared_ptr<BucketBundle>> mainBuckets(N1), crBuckets(N1);
	int size = N1;

	for (int i = 0; i<N1; i++) {

		mainBuckets[i] = BucketBundleList::loadBucketFromFile(BUCKETS_PREFIX_MAIN + "." + to_string(BUCKET_ID) + ".cbundle");
		crBuckets[i] = BucketBundleList::loadBucketFromFile(BUCKETS_PREFIX_CR + "." + to_string(BUCKET_ID++) + ".cbundle");
	}

    auto input = CircuitInput::fromFile(CIRCUIT_INPUT_FILENAME, mainBuckets[0]->getBundleAt(0)->getNumberOfInputLabelsX());
	// only now we start counting the running time
	string tmp = "reset times";
	cout << "tmp size = " << tmp.size() << endl;
	byte tmpBuf[20];

    OnlineProtocolP1* protocol = new OnlineProtocolP1();
	vector<long long> times(size);
    for (int j = 0; j < 10; j+=4) {
		//cout << "num of threads = " << j << endl;
		CryptoPrimitives::setNumOfThreads(j);

		for (int i = 0; i < size; i++) {
			protocol->getChannel()->write((const byte*)tmp.c_str(), tmp.size());
			int readsize = protocol->getChannel()->read(tmpBuf, tmp.size());

			auto mainBucket = mainBuckets[i];
			auto crBucket = crBuckets[i];
			protocol->setBuckets(*mainBucket, *crBucket);
            protocol->setInput(input);

            auto start = chrono::high_resolution_clock::now();

			protocol->run();
            auto end = chrono::high_resolution_clock::now();
			auto time = chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
			//cout << "exe no. " << i << " took " << time << " millis." << endl;
			times[i] = time;
		}

		int count = 0;
		for (int i = 0; i < size; i++) {
			count += times[i];
            cout <<times[i] << " ";
		}

		auto average = count / size;

		cout << endl;

		//System.out.println();
		cout << size << " executions took in average" << average << " milis." << endl;
	}

    delete protocol;

	cout << "\nP1 end communication\n";

	return 0;
}