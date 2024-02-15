//
//  PI.cpp
//  verifier
//
//  Created by Florian on 15.11.23.
//

#include "PI.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <array>
#include <iterator>
#include <cstring>
#include <chrono>

#include "gaussian-elimination/GaussianElimination.hpp"
#include "gaussian-elimination/PlainGaussHelper.hpp"

#include "Matrix.hpp"

using namespace Gauss;
using namespace PI;
using namespace std;
using namespace Matrix;

using std::chrono::high_resolution_clock;
using std::chrono::duration;

namespace fs = std::filesystem;

Verifier::Verifier(VerifierContext *ctx): ctx(ctx) {
}

Result Verifier::Verify() {
    Result res;
    std::vector<std::string> logFiles = getAllLogFiles(ctx->logFileDirectory, LOG_EXTENSION);

    for (const auto &logFile : logFiles) {
        cout << "Start verifying: " << logFile << endl;
        Verifier::verifySingleLogFile(logFile, ctx->outFile);
    }
    
    return res;
}

// here is where the magic happens.
Result Verifier::verifySingleLogFile(std::string path, std::string resultPath) {
    Result res {1, true};
    std::unordered_map<ID_TYPE, KeyStoreEntry> KeyStore;
    vector<Keys> keys;
    std::vector<Tau_i> Tau(ctx->m);
    std::unordered_map<int, array<int, K>> drns;
    KEY_TYPE Ki, encKey, drnKey, tagKey, idKey, k0;
    const XOR_TYPE nullVector = {0};
    
    std::ifstream logFile(path, std::ios::binary); // this is our log file :*
    int rank = 0; // line 10
    BMatrixType *M;
    
    if (!logFile.is_open()) {
        std::cerr << "Could not open file log file." << std::endl;
        exit(EXIT_FAILURE);
    }
    
    k0 = readMasterKey(ctx->masterKeyPath);
    Ki = k0;
    
    // generate all possible n keys
    // line 1
    for (int i = 1; i <= ctx->n; ++i) {
        std::array<int, K> kRandom;
        // generate the ith keye.
        // line 2
        if (0 == KeyEvolution(Ki.data(), Ki.data())){
            std::cerr << "ERROR: Key Evolution failed." << std::endl;
            exit(EXIT_FAILURE);
        }
        // derive all sub keys, and store them.
        if (0 == DeriveSubKeys(Ki.data(), encKey.data(), drnKey.data(), tagKey.data(), idKey.data()))
        {
            std::cerr << "Error: Failed to derive sub keys." << std::endl;
            exit(EXIT_FAILURE);
        }
        Keys ks = {Ki, encKey, drnKey, tagKey, idKey};
        
        keys.push_back(ks);
        
        // re generate the k distinct random locations.
        // line 3
        if (0 == DRN(drnKey.data(), K, ctx->m, kRandom.data())){
            std::cerr << "Error: Failed to create k distinct random numbers." << std::endl;
            exit(EXIT_FAILURE);
        }
        // store them.
        drns[(i-1)] = kRandom;
        
        // regenerate all key IDs for each of the k locations.
        // line 4
        for (int j = 0; j < K; ++j) {
            array<unsigned char, ID_LEN> ID;
            int lj = kRandom[j];
            
            // generate the ID.
            // line 5
            if (0 == CreateID(idKey.data(), j, ID.data())) {
                std::cerr << "Error: Failed to createID." << std::endl;
                exit(EXIT_FAILURE);
            }
            // store the k IDs with the associated key, log iteration index, and the respective location within the log file.
            // line 5
            KeyStoreEntry newEntry = {ks, i, lj};
            
            KeyStore[ID] = newEntry;
        }
    }
    
    // Predict M's rank:
    std::array<unsigned char, LOG_LEN>log;//(ID_LEN);
    int j;
    // line 11
    for (int i = 0; i < ctx->m; ++i) {
        // get the entry Tau_i from the log file.
        logFile.seekg(i * LOG_LEN, std::ios::beg);
        logFile.read(reinterpret_cast<char*>(log.data()), LOG_LEN);
        if (!logFile) {
            std::cerr << "Error: reading from log file. Consider choosing right amount for N." << std::endl;
            exit(EXIT_FAILURE);
        }
        // parse the entry Tau_i
        Tau_i taui;
        
        std::copy(log.begin(), log.begin() + CIPHERTEXT_LEN, taui.XOR.begin());
        std::copy(log.begin() + CIPHERTEXT_LEN, log.begin() + CIPHERTEXT_LEN + INTEGRITY_TAG_LEN, taui.T.begin());
        std::copy(log.begin() + CIPHERTEXT_LEN + INTEGRITY_TAG_LEN, log.begin() + CIPHERTEXT_LEN + INTEGRITY_TAG_LEN + ID_LEN, taui.ID.begin());
        
        Tau[i] = taui;
        
        // check if the ID can be found in the KeyStore, and if it is found, check wether it is the current highest number.
        // line 12
        auto it = KeyStore.find(taui.ID);
        if (KeyStore.end() == it) {
            continue; // NEXT
        }
        
        j = it->second.i;
        // line 13
        rank = max(rank, j);
    }
    
    // check if rank is larger 0
    // line 15
    if (rank == 0) {
        cerr << "ERROR: Rank is 0." << endl;
        exit(EXIT_FAILURE);
    }
    
    cout << "Detected " << rank << " different log entries." << endl;
    
    // Create M=m x n zero Matrix over GF(2).
    // line 9
    M = new BMatrixType(ctx->m, rank);
	
    // null all vectors in the log file, which have been tampered, to avoid them corrupting the output.
    // line 16
    for (int i = 0; i < rank; ++i) {
        // line 18
        for (int j = 0; j < K; ++j) {
            // get the k distinct random locations, for the ith log iteration.
            // line 17
            int lj = drns[i][j];
            // line 18
            KeyStoreEntry kse = KeyStore[Tau[lj].ID];
            TAG_TYPE _T;
            
            // create the integrity tag based on the XOR part
            // line 20
            if (0 == CreateIntegrityTag(kse.Ki.TagKey.data(), Tau[lj].XOR.data(), _T.data())) {
                cerr << "ERROR: Failed to create the integrity tag." << endl;
                exit(EXIT_FAILURE);
            }
            // check wether the vector has been tampered
            // line 20
            if (kse.lj == lj && _T == Tau[lj].T) {
                // toggle the bit in M
                // line 20
                M->setBit(lj, i);// data[lj * M->buckets + i] = 1;
                continue;
            }
            // check if it has been nulled already, because the locations could be check severall times.
            if (Tau[lj].XOR != nullVector) {
                cout << "Line '" << lj << "' has been tampered." << endl;
            }
            
            // null the tampered vector.
            // line 21
            std::fill(Tau[lj].XOR.begin(), Tau[lj].XOR.end(), 0);
            // set basic tampering indicator
            res.success = false;
            res.code = 0;
        }
    }
    
    
    // Remove the random PAD.
    // line 23
    PRGContext *prgCtx = CreatePRGContext(k0.data());
    std::array<unsigned char, LOG_LEN> randomPad;
    
    // line 24
    for (int i = 0; i < ctx->m; ++i) {
        // line 25
        if (-1 == PRG(prgCtx, randomPad.data(), LOG_LEN)) {
            delete prgCtx;
            cerr << "ERROR: Creating random PAD." << endl;
            exit(EXIT_FAILURE);
        }
        // It is important to generate the PRG output first, and abort the iteration after, otherwise the internal state (counter) differs from the original state.
        
        // Check if the XOR part has been nulled.
        // line 25
        if (Tau[i].XOR == nullVector)
            continue;
        
        // remove random pad of the XOR part of Tau[i].
        // line 25
        std::transform(Tau[i].XOR.begin(), Tau[i].XOR.end(), randomPad.begin(), Tau[i].XOR.begin(), [](unsigned char a, unsigned char b){
            return a ^ b;
        });
    }
    
    delete prgCtx;
    
    std::vector<XOR_TYPE> v;
    v.reserve(Tau.size());

    // Use std::transform to extract XOR elements into a new vector
    std::transform(Tau.begin(), Tau.end(), std::back_inserter(v),
        [](const Tau_i& tau) { return tau.XOR; });
    
    std::vector<PI::XOR_TYPE> c;
    // choose metal or CPU:
    if (ctx->useMetal) {
        auto t1 = high_resolution_clock::now();
        GaussianElimination ge (M, v);
        // solve gauss, and get the cipher text vector c
        // line 27
        c = ge.solve();
        auto t2 = high_resolution_clock::now();
        
        duration<long, std::nano> ns_double = t2 - t1;
        
        cout << "Gaussian Elimination (Metal): " << ns_double.count() << " ns." << endl;
    } else {
        auto t1 = high_resolution_clock::now();
        // solve gauss, and get the cipher text vector c
        // line 27
        c = PlainGaussHelper::Solve(M, v, false);
        auto t2 = high_resolution_clock::now();
        
        duration<long, std::nano> ns_double = t2 - t1;
        
        cout << "Gaussian Elimination (CPU): " << ns_double.count() << " ns." << endl;
    }
    
    std::ofstream resultLogFile(resultPath, std::ios::app);
    // Check if the file is successfully opened
    if (!resultLogFile.is_open()) {
        std::cerr << "ERROR: opening the result file!" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    // decrypt the log files, and check the MACs.
    // line 27
    for (int i = 0; i < rank; ++i) {
        // decrypt the log message
        //line 29
        string log = decryptLog(keys[i].EncKey, c[i]);
        
        // check wether the log message has been tampered.
        // line 30
        if (log == "") {
            continue;
        }
        // write the log into the desired file.
        resultLogFile << log << endl;
     }
    
    resultLogFile.close();
    return res;
}

string Verifier::decryptLog(KEY_TYPE key, XOR_TYPE encLogMessage)
{
    array<unsigned char, IV_SIZE> iv;
    array<unsigned char, MESSAGE_LEN> ciphertext;
    array<unsigned char, MESSAGE_LEN> logm{0};
    array<unsigned char, MAC_LEN> mac, referenceMAC;
    size_t macLen, len;
    
    
    copy_n(encLogMessage.begin(), IV_SIZE, iv.begin());
    copy_n(encLogMessage.begin() + IV_SIZE, MESSAGE_LEN, ciphertext.begin());
    copy_n(encLogMessage.begin() + IV_SIZE + MESSAGE_LEN, MAC_LEN, mac.begin());
    
    
    CMAC(key.data(), ciphertext.data(), MESSAGE_LEN, referenceMAC.data(), &macLen, MAC_LEN);
    
    if (macLen != MAC_LEN) {
        cerr << "ERROR: Creating MAC failed." << endl;
        exit(EXIT_FAILURE);
    }
    
    if (mac != referenceMAC){
        // TODO: improve messageing:
        cerr << "Invalid MAC detected: ..." << endl;
        return "";
    }
    
    if (MESSAGE_LEN != (len = AES_256_CTR_decrypt
                        (ciphertext.data(), MESSAGE_LEN, key.data(), iv.data(), logm.data()))) {
        cerr << "ERROR: Log encryption failed." << endl;
        exit(EXIT_FAILURE);
    }
    // Return log.
    std::string s(logm.data(), std::find(logm.begin(), logm.end(), '\0'));
    return s;
}


std::vector<std::string> Verifier::getAllLogFiles(const std::string& directoryPath, const std::string& extension) {
    std::vector<std::string> filePaths;
    
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (fs::is_regular_file(entry.path())) {
            std::string fileName = entry.path().filename().string();
            if (fileName.size() >= extension.size() && fileName.compare(fileName.size() - extension.size(), extension.size(), extension) == 0) {
                filePaths.push_back(entry.path().string());
            }
        }
    }

    return filePaths;
}


KEY_TYPE Verifier::readMasterKey(std::string path) {
    KEY_TYPE mk;
    
    if (fs::exists(path)) {
        std::ifstream masterKeyFile(path, std::ios::binary);
        
        if (masterKeyFile.is_open()) {
            if (masterKeyFile.read(reinterpret_cast<char*>(&mk), KEY_SIZE)) {
                // Process the read byte
                return mk;
            }
        }
        std::cerr << "ERROR: Failed to open Master Key file for reading." << std::endl;
    }
    
    std::cerr << "ERROR: Master Key file not found." << std::endl;
    return mk;
}

