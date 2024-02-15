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

#define MESSAGE_LEN 1024 // The max len of an log entry message. (l) has to be a multiple of the AES block length (!)
#define INTEGRITY_TAG_LEN 16 // The integrity tag len.
#define ID_LEN 16 // The ID (to find the correct key for this log entry) len.
#define MAC_LEN 16 // Encrypt then MAC HMAC len.
#define CIPHERTEXT_LEN (IV_SIZE + MESSAGE_LEN + MAC_LEN)
#define LOG_LEN (CIPHERTEXT_LEN + INTEGRITY_TAG_LEN + ID_LEN)

typedef struct {
    unsigned char sessionKey[KEY_SIZE]; // Contains the current session key.
    char *keyPath; // Path to the current seassion key.
    const char *logFileDirectory; // Directory holding the log files.
    const char *logFileNamePrefix; // Name of the log file.
    unsigned long maxEntries; // (n) Maximum number of logs the file could hold.
    int m; // m = m * c.
    FILE *logFile; // File pointer of the log file.
    FILE *keyFile; // File pointer of the session key file.
} PIContext;

/*
 * Function: CreatePIContext
 * -------------------------
 * Will create a struct of type PIContext, based on the provided informations.
 *
 * n : the maximum number of log entries, that can be stored in a single file.
 * keyPath : Path to the directory containing the (master)key file.
 * logFilePath: Path to the log file directory.
 * logFilePrefix: Log file name.
 *
 * returns: a new struct of type PIContext, initialized with the requested parameters.
 */
PIContext *CreatePIContext(unsigned long n, const char *keyPath, const char *logFilePath, const char* logFilePrefix);

/*
 * Function: Init
 * --------------
 * Based on the provided context a new log file will be created.
 *
 * ctx: Logger Context.
 */
void Init(PIContext *ctx);

/*
 * Function: AddLogEntry
 * ---------------------
 * Will add a new log entry to the log file.
 *
 * ctx:  Logger Context.
 * logMessage: Log message that will be logged.
 * logMessageSize: size of the message, messages longer than the max log legth, will be truncated.
 *
 * returns: 0 on failure and 1 on sucess.
 */
int AddLogEntry(PIContext *ctx, unsigned char *logMessage, int logMessageSize);

/*
 * Function: Readkey
 * -----------------
 * Will read the current key from the file.
 *
 * path: Path to the key file.
 * key: The key will be written here.
 *
 * returns: 0 on failure and 1 on success.
 */
int Readkey(char *path, unsigned char key[KEY_SIZE]);
#endif /* PI_h */
