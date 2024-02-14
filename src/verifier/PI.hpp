//
//  PI.hpp
//  verifier
//
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
        Verifier(PI::VerifierContext *ctx);
        Result Verify();
        
        
        //~Verifier();
    private:
        PI::VerifierContext *ctx;
        std::string decryptLog (KEY_TYPE key, XOR_TYPE encLogMessage);
        std::vector<std::string> getAllLogFiles(const std::string& directoryPath, const std::string& extension);
        Result verifySingleLogFile(std::string path, std::string resultPath);
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

/*
class PI{
public:
    static Result Verify();
};
*/
#endif /* PI_hpp */
