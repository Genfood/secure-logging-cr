//
//  PI.h
//  logger
//
//  Created by Florian on 27.10.23.
//

#ifndef PI_h
#define PI_h

#include "PIShared.h"
#include <stdio.h>
#include "Crypto.h"

#define MESSAGE_LEN 1024 // The max len of an log entry message.
#define INTEGRITY_TAG_LEN 16 // The integrity tag len.
#define ID_LEN 16 // The ID (to find the correct key for this log entry) len.
#define MAC_LEN 16 // Encrypt then MAC HMAC len. ??CMAC??
#define CIPHERTEXT_LEN (IV_SIZE + MESSAGE_LEN + MAC_LEN)
#define LOG_LEN (CIPHERTEXT_LEN + INTEGRITY_TAG_LEN + ID_LEN)

typedef struct {
    unsigned char sessionKey[KEY_SIZE];
    char *keyPath;
    const char *logFileDirectory;
    const char *logFileNamePrefix;
    unsigned long maxEntries; // n
    unsigned long m;
    FILE *logFile;
    FILE *keyFile;
} PIContext;

/*
 n : the maximum number of log entries, that can be stored in a single file.
 s : Security Parameter.
 */
PIContext *CreatePIContext(unsigned long n, const char *keyPath, const char *logFilePath, const char* logFilePrefix);

void Init(PIContext *ctx);

int AddLogEntry(PIContext *ctx, unsigned char *logMessage, int logMessageSize);

int Readkey(char *path, unsigned char key[KEY_SIZE]);
#endif /* PI_h */
