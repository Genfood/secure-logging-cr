//
//  Crypto.h
//  shared
//
//  Created by Florian on 15.11.23.
//

#ifndef Crypto_h
#define Crypto_h
#include <stdio.h>
#include <openssl/evp.h>

#define AES_BLOCK_LEN 16
#define KEY_SIZE 32
#define IV_SIZE 16

// crypto constants with a large hamming distance
// 4 constants of 1 byte, with a Hamming Distance of 4
#define PAD1 0xd
#define PAD2 0x3b
#define PAD3 0x51
#define PAD4 0xcb

static unsigned char GAMMA[2 * AES_BLOCK_LEN] = {[0 ... (2 * AES_BLOCK_LEN -1)] = PAD1};
static unsigned char GAMMA_DASH[AES_BLOCK_LEN] = {[0 ... (AES_BLOCK_LEN -1)] = PAD2};

// PRF stuff
typedef struct PRG128Context{
    unsigned int counter;
    unsigned char seed[16];
} PRG128Context;

typedef struct PRGContext{
    unsigned int counter;
    unsigned char seed[KEY_SIZE];
} PRGContext;

PRGContext *CreatePRGContext(unsigned char seed[KEY_SIZE]);
PRG128Context *CreatePRG128Context(unsigned char seed[16]);
int PRG(PRGContext *ctx, unsigned char *buffer, int size);
int PRG128(PRG128Context *ctx, unsigned char *buffer, int size);

// key stuff
int KeyEvolution(unsigned char *key, unsigned char *nextKey);
int DeriveSubKeys(unsigned char masterSessionkey[KEY_SIZE], unsigned char encKey[KEY_SIZE], unsigned char drnKey[KEY_SIZE], unsigned char tagKey[KEY_SIZE], unsigned char idKey[KEY_SIZE]);
int GenerateMasterKey(unsigned char *masterKey);
int GenerateIV(unsigned char *iv);

// PRF
int PRF(unsigned char *input, size_t inputSize, unsigned char *key, unsigned char *output, uint8_t outputSize);

// Random things
unsigned int UniformRandomInt(PRGContext *ctx, const unsigned int upperBound);
int DRN(unsigned char *seed, int k, const int upperBound, int *kRandom);

// AES stuff
int AES_256_CTR_encrypt(unsigned char *plaintext, int plaintextSize, unsigned char *key, unsigned char *iv, unsigned char *ciphertextBuffer);
int AES_256_CTR_decrypt(unsigned char *ciphertext, int ciphertextSize, unsigned char *key, unsigned char *iv, unsigned char *plaintextBuffer);

int CMAC(unsigned char *key, unsigned char *input, size_t inputSize, unsigned char *output, size_t *outputSize, size_t maxOutputSize);

#endif /* Crypto_h */
