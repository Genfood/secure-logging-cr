//
//  PI.hpp
//  verifier
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 15.11.23.
//

#ifndef PI_hpp
#define PI_hpp

#include <stdio.h>
#include <string.h>
#include <vector>
#include <array>

#include "Result.hpp"
#include "PITypes.hpp"


typedef std::basic_string<unsigned char> ustring;
namespace PI {
    class Verifier {
    public:
        /*
         * Contructor
         * ----------
         */
        Verifier(PI::VerifierContext *ctx);
        /*
         * Function: Verify
         * ----------------
         * Verifies the provided log file.
         */
        Result Verify();
    private:
        PI::VerifierContext *ctx; // the current context
        /*
         * Function: decryptLog
         * --------------------
         * Decrypts the provided log message.
         */
        std::string decryptLog (KEY_TYPE key, XOR_TYPE encLogMessage);
        /*
         * Function: getAllLogFiles
         * not part of this thesis!
         * will only work with a single log file.
         */
        std::vector<std::string> getAllLogFiles(const std::string& directoryPath, const std::string& extension);
        /*
         * Function: verifySingleLogFile
         * -----------------------------
         * verifies the provided log file.
         */
        Result verifySingleLogFile(std::string path, std::string resultPath);
        /*
         * Function: readMasterKey
         * -----------------------------
         * Returns the master key given by path.
         */
        KEY_TYPE readMasterKey(std::string path);
    };
}

// Hash function for std::array
namespace std {
    template<typename T, size_t N>
    struct hash<array<T, N>> {
        size_t operator()(const array<T, N>& arr) const {
            size_t hash_val = 0;
            for (const auto& element : arr) {
                // https://stackoverflow.com/a/42701911/5102753
                hash_val ^= std::hash<T>{}(element) + 0x9e3779b9 + (hash_val << 6) + (hash_val >> 2);
            }
            return hash_val;
        }
    };
}
#endif /* PI_hpp */
