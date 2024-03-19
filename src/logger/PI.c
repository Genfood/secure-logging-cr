//
//  PI.c
//  logger
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 27.10.23.
//

#include "PI.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <math.h>

#define SESSION_KEY

// Prototype decleration
static int createNewLogFile(FILE *file, unsigned long fileSize);
static void initializeLogFileWithPseudoRandomPad(PRGContext *ctx, FILE *file, size_t m);
static int writePRGToFile(PRGContext *ctx, FILE *file, unsigned char *buffer, int bufferSize);
static int updateKey(PIContext *ctx);
static int writeKey(unsigned char key[KEY_SIZE], char *path);
static int encryptLog(unsigned char *key, unsigned char *logMessage, int logMessageSize, unsigned char *cipherLogMessage);

int AddLogEntry(PIContext *ctx, unsigned char *logMessage, int logMessageSize)
{
    unsigned char encKey[KEY_SIZE], drnKey[KEY_SIZE], tagKey[KEY_SIZE], idKey[KEY_SIZE],
        cipherLogMessage[MESSAGE_LEN + IV_SIZE + MAC_LEN],
        TauiBuffer[LOG_LEN],
        XORlj[MESSAGE_LEN + IV_SIZE + MAC_LEN],
        Tlj[INTEGRITY_TAG_LEN],
        IDlj[ID_LEN];
    
    int kRandom[K];
    
    // derive keys
    if (0 == DeriveSubKeys(ctx->sessionKey, encKey, drnKey, tagKey, idKey))
    {
        perror("Error: Failed to derive sub.\n");
        return 0;
    }
    
    
    // cipherLogMessage = malloc(MESSAGE_LEN + IV_SIZE + MAC_LEN);
    // line 1
    if (0 == encryptLog(encKey, logMessage, logMessageSize, cipherLogMessage)) {
        perror("Error: Failed to encrypt the log message.\n");
        
        return 0;
    }
    
    // Create the k distinct random locations within the number m (which is the log file "length").
    // line 2
    if (0 == DRN(drnKey, K, ctx->m, kRandom)){
        perror("Error: Failed to create k distinct random numbers.\n");
        return 0;
    }
    
    // XOR ci (the encrypted log file) at the k distinct random locations within the log file.
    // line 4
    int l;
    for (int j = 0; j < K; ++j) {
        l = kRandom[j];
        
        // seek to the requiered location within the log file
        if (fseek(ctx->logFile, l * LOG_LEN, SEEK_SET) != 0) {
            printf("Error: Unable to move the file position indicator.\n");
            
            return 0;
        }
        
        // Check for read errors or end-of-file
        if (ferror(ctx->logFile)) {
            printf("Error: reading from the file.\n");
            
            int errnum = errno;
            fprintf(stderr, "Error opening file: %s\n", strerror(errnum));
            return 0;
        }
        
        // read the location from the log file.
        if (fread(TauiBuffer, 1, LOG_LEN, ctx->logFile) != LOG_LEN) {
            printf("Error: reading from the file.\n");
            int errnum = errno;
            fprintf(stderr, "Error opening file: %s\n", strerror(errnum));
            
            return 0;
        }
    
        // cipher ⊕ XORlj
        // XOR the XOR parts.
        // line 5
        for (int i = 0; i < CIPHERTEXT_LEN; ++i) {
            XORlj[i] = TauiBuffer[i] ^ cipherLogMessage[i];
        }
        
        // create integrity TAG
        // line 6
        if (0 == CreateIntegrityTag(tagKey, XORlj, Tlj)) {
            printf("Error: Failed to create the integrity tag.\n");
            
            return 0;
        }
        
        // create ID
        // line 7
        if (0 == CreateID(idKey, j, IDlj)) {
            printf("Error: Failed to create ID.\n");
            
            return 0;
        }
        
        // seek to the position again
        if (fseek(ctx->logFile, l * LOG_LEN, SEEK_SET) != 0) {
            printf("Error: Unable to move the file position indicator.\n");
            
            return 0;
        }
        
        memcpy(TauiBuffer, XORlj, CIPHERTEXT_LEN);
        memcpy(TauiBuffer + CIPHERTEXT_LEN, Tlj, INTEGRITY_TAG_LEN);
        memcpy(TauiBuffer + CIPHERTEXT_LEN + INTEGRITY_TAG_LEN, IDlj, ID_LEN);
        
        // write back to file
        if (fwrite(TauiBuffer, 1, LOG_LEN, ctx->logFile) != LOG_LEN) {
            printf("Error: Failed to write the XORed log message back.\n");
            
            return 0;
        }
    }
    
    // key evolution
    // line 9
    updateKey(ctx);
    
    return 1;
}

void Init(PIContext *ctx)
{
    size_t fileSize = ctx->m * LOG_LEN; // m * LOG_LEN
    // line 2
    if (createNewLogFile(ctx->logFile, fileSize) == 0){
        /* Failed to create new log file. */
        perror("ERROR: Failed to initiate the log file, with the given size.");
        exit(EXIT_FAILURE);
    }
    
    // declare context outside of the file preperation, to reuse the given context if more then one file will be filled up. (not in this thesis)
    PRGContext *prgContext = CreatePRGContext(ctx->sessionKey);
    if (prgContext == NULL) {
        perror("ERROR: Failed to create the prg Context.");
        exit(EXIT_FAILURE);
    }
    // line 4
    // write random pad.
    initializeLogFileWithPseudoRandomPad(prgContext, ctx->logFile, ctx->m);
    
    free(prgContext);
    
    // First key evolution.
    // line 6
    updateKey(ctx);
}

PIContext *CreatePIContext(unsigned long n, const char *keyPath, const char *logFileDir, const char* logFilePrefix)
{
    PIContext *ctx = calloc(1, sizeof(PIContext));
    unsigned char *masterKey = calloc(KEY_SIZE, sizeof(unsigned char));
    // TODO: MasterKey generation should happen outside of the logging...
    char masterKeyfileName[] = "masterKey.key";
    char masterKeyPath[strlen(keyPath) + strlen(masterKeyfileName) + 1];
    char sessionKeyfileName[] = "currentSessionKey.key";
    char *sessionKeyPath = calloc(strlen(keyPath) + strlen(sessionKeyfileName) + 1, sizeof(char));
    
    // prepate master key path
    // TODO: MasterKey generation should happen outside of the logging...
    strcpy(masterKeyPath, keyPath);
    strcat(masterKeyPath, masterKeyfileName);
    // prepare session key path for context
    strcpy(sessionKeyPath, keyPath);
    strcat(sessionKeyPath, sessionKeyfileName);
    
    
    if(access(sessionKeyPath, F_OK) == 0)
    {
        // a session key already exists.
        printf("INFO: A session key has been found.\n");
        unsigned char *sessionKey = calloc(KEY_SIZE, sizeof(unsigned char));
        
        Readkey(sessionKeyPath, sessionKey);
        
        memcpy(ctx->sessionKey, sessionKey, KEY_SIZE);
        free(sessionKey);
    }
    else
    {
        // No session key exists.
        printf("INFO: No session key has been found.\n");
        printf("INFO: Generate new Master Key.\n");
        GenerateMasterKey(masterKey);
        
        // store master key
        // TODO: MasterKey generation should happen outside of the logging...
        writeKey(masterKey, masterKeyPath);
        // Set key0
        memcpy(ctx->sessionKey, masterKey, KEY_SIZE);
    }
    
    ctx->keyPath = sessionKeyPath;
    ctx->logFileDirectory = logFileDir;
    ctx->maxEntries = n;
    ctx->m = ceil(n * C);
    ctx->logFileNamePrefix = logFilePrefix;
    
    
    // Create Key File
    ctx->keyFile = fopen(ctx->keyPath, "wb");
    
    if (ctx->keyFile == NULL) {
        perror("ERROR: Failed to open the key file.");
        exit(EXIT_FAILURE);
    }
    
    // TODO: multiple files. Not in this thesis.
    // create log file and open it:
    char *logFilePath = calloc(strlen(ctx->logFileDirectory) + strlen(ctx->logFileNamePrefix) + strlen(LOG_EXTENSION) + 1, sizeof(char));
    
    strcpy(logFilePath, ctx->logFileDirectory);
    strcat(logFilePath, ctx->logFileNamePrefix);
    strcat(logFilePath, LOG_EXTENSION);
    
    if ((ctx->logFile = fopen(logFilePath, "wb+")) == NULL) {
        perror("ERROR: Failed to open the log file.\n");
        int errnum = errno;
        fprintf(stderr, "ERROR: opening file: %s\n", strerror(errnum));
               
        free(logFilePath);
        exit(EXIT_FAILURE);
    }
    free(logFilePath);
    
    return ctx;
}

// helper method:
// create a new log file of the requested size.
static int createNewLogFile(FILE *file, unsigned long fileSize)
{
    if (file == NULL) {
        perror("ERROR: File is not opened.");
        return 0;
    }
        
    // Seek to file size
    if (fseek(file, fileSize - 1, SEEK_SET) != 0) {
        perror("Error seeking to the end of the file");
        fclose(file);
        return 0;
    }
    
    // Write a single byte to the end of the file to allocate space
    fputc('\0', file);
    
    
    return 1;
}

// helper function:
// writes the random pad into the empty file.
static void initializeLogFileWithPseudoRandomPad(PRGContext *ctx, FILE *file, size_t m)
{
    unsigned char buffer[LOG_LEN];
    
    fseek(file, 0, SEEK_SET);
    // write random pad "line" by "line".
    for (int i = 0; i < m; ++i) {
        if (writePRGToFile(ctx, file, buffer, LOG_LEN) != 1) {
            perror("ERROR: Failed to write the random pad.");
            exit(EXIT_FAILURE);
        }
    }
}

// helper function
// writes a specific amount of pseudo random data into the provided file.
static int writePRGToFile(PRGContext *ctx, FILE *file, unsigned char *buffer, int bufferSize)
{
    // Create pseudo random data.
    if (PRG(ctx, buffer, bufferSize) == -1)
    {
        perror("ERROR: Creating random PAD.\n");
        return -1;
    }
    
    // write the random data into the file.
    size_t bytesWritten = fwrite(buffer, 1, bufferSize, file);
    
    if (bytesWritten != bufferSize)
    {
        perror("ERROR: While writing pseudo random PAD to the file.\n");
        fclose(file);
        return -1;
    }
    return 1;
}

// helper function:
// key evolution + updating the key file.
static int updateKey(PIContext *ctx)
{
    unsigned char nextKey[KEY_SIZE];
    KeyEvolution(ctx->sessionKey, nextKey);
    
    fseek(ctx->keyFile, 0, SEEK_SET);
    
    size_t writtenT = fwrite(nextKey, 1, KEY_SIZE, ctx->keyFile);
    
    if (writtenT != KEY_SIZE) {
        perror("ERROR: Failed to write the key to the file.");
        
        exit(EXIT_FAILURE);
    }

    
    memcpy(ctx->sessionKey, nextKey, KEY_SIZE);
    return 1;
}

int Readkey(char *path, unsigned char key[KEY_SIZE])
{
    FILE *file = fopen(path, "rb");
    
    if(!file)
    {
        perror("Failed to open the key file.");
        return 0;
    }
    
    if(fread(key, 1, KEY_SIZE, file) != KEY_SIZE)
    {
        perror("Failed to read the key.");
        fclose(file);
        return 0;
    }
    
    fclose(file);
    return 1;
}

static int writeKey(unsigned char key[KEY_SIZE], char *path)
{
    FILE *file = fopen(path, "wb");
    
    if (file == NULL) {
        perror("Failed to open the key file.");
        return 0;
    }
    
    size_t writtenT = fwrite(key, 1, KEY_SIZE, file);
    
    if (writtenT != KEY_SIZE) {
        perror("Failed to write the key to the file");
        fclose(file);
        return 0;
    }
    
    fclose(file);
    return 1;
}

/*
 * Function: encryptLog
 * --------------------
 * Will AE$ encrypt the provided log message, by uisng AES_256_CTR + CMAC.
 *
 * key: the current session key for encryption.
 * logMessage: the log message to encrypt.
 * logMessageSize: the size of the log message.
 * cipherLogMessage: (ci) the ciphertext, always of the size IV + MESSAGE_LEN + MAC_LEN.
 *
 * return: 0 on failure and IV_SIZE + MESSAGE_LEN + MAC_LEN on success.
 */
static int encryptLog(unsigned char *key, unsigned char *logMessage, int logMessageSize, unsigned char *cipherLogMessage)
{
    unsigned char iv[IV_SIZE];
    unsigned char paddedMessage[MESSAGE_LEN];
    unsigned char ciphertext[MESSAGE_LEN];
    unsigned char mac[MAC_LEN];
    int len;
    size_t macLen;
    
    GenerateIV(iv);
    
    // padding with 0 bytes.
    memset(paddedMessage, 0, sizeof(paddedMessage));
    memcpy(paddedMessage, logMessage, logMessageSize);
    
    // encrypt the log message.
    if (MESSAGE_LEN != (len = AES_256_CTR_encrypt(paddedMessage, MESSAGE_LEN, key, iv, ciphertext)))
    {
        perror("ERROR: Log encryption failed.\n");
        return 0;
    }
    
    memcpy(cipherLogMessage, iv, IV_SIZE);
    memcpy(cipherLogMessage + IV_SIZE, ciphertext, MESSAGE_LEN);
    
    // Create the MAC.
    CMAC(key, ciphertext, MESSAGE_LEN, mac, &macLen, MAC_LEN);
    
    memcpy(cipherLogMessage + IV_SIZE + MESSAGE_LEN, mac, MAC_LEN);
    
    // Return cipherLogMessage len.
    return IV_SIZE + MESSAGE_LEN + MAC_LEN;
}
