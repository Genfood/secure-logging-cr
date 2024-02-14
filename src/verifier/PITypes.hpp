//
//  PITypes.hpp
//  verifier
//
//  Created by Florian on 21.11.23.
//

#ifndef PITypes_hpp
#define PITypes_hpp

#include <string>
extern "C" {
    #include "Crypto.h"
    #include "PIShared.h"
}
#include <stdio.h>

namespace PI {
    typedef struct _VerifierContext {
        std::string logFileDirectory;
        std::string outFile;
        std::string masterKeyPath;
        int n; // max number of log entries.
        int m;
        bool useMetal;
    } VerifierContext;
    
    typedef std::array<unsigned char, KEY_SIZE> KEY_TYPE;
    typedef struct _Keys{
        KEY_TYPE Key;
        KEY_TYPE EncKey;
        KEY_TYPE DrnKey;
        KEY_TYPE TagKey;
        KEY_TYPE IDKey;
    } Keys;
    
    typedef struct _KeyStoreEntry{
        Keys Ki;
        int i;
        int lj;
    } KeyStoreEntry;
    
    typedef std::array<unsigned char, ID_LEN> ID_TYPE;
    typedef std::array<unsigned char, INTEGRITY_TAG_LEN> TAG_TYPE;
    typedef std::array<unsigned char, CIPHERTEXT_LEN> XOR_TYPE;
    typedef std::array<unsigned char, MESSAGE_LEN> LOG_MESSAGE_TYPE;
    /*
     struct that stores one log entry of the encrypted log file in it.
     */
    typedef struct _Tau_i {
        XOR_TYPE XOR;
        TAG_TYPE T;
        ID_TYPE ID;
    } Tau_i;
    
}


#endif /* PITypes_hpp */
