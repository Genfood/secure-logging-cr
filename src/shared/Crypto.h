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
    unsigned int counter; // internal counter, which will be increased for each AES block added within the current context.
    unsigned char seed[16]; // secure seed.
} PRG128Context;

typedef struct PRGContext{
    unsigned int counter; // internal counter, which will be increased for each AES block added within the current context.
    unsigned char seed[KEY_SIZE]; // secure seed.
} PRGContext;

PRGContext *CreatePRGContext(unsigned char seed[KEY_SIZE]);
PRG128Context *CreatePRG128Context(unsigned char seed[16]);

/*
 * Function: PRG
 * -------------
 * A AES_256_CTR based Pseudo Random Generator.
 *
 * ctx:  PRG Context.
 * buffer: which will hold the random data.
 * size: the amount of random data, and the size of the buffer.
 *
 * returns: 0 on failure and 1 on success.
 */
int PRG(PRGContext *ctx, unsigned char *buffer, int size);

/*
 * Function: PRG
 * -------------
 * A AES_128_CTR based Pseudo Random Generator.
 *
 * ctx:  PRG Context.
 * buffer: which will hold the random data.
 * size: the amount of random data, and the size of the buffer.
 *
 * returns: 0 on failure and 1 on success.
 */
int PRG128(PRG128Context *ctx, unsigned char *buffer, int size);

// key stuff:

/*
 * Function: KeyEvolution
 * ----------------------
 * K_{i+1} = PRF_{K_i}(\gamma)
 */
int KeyEvolution(unsigned char *key, unsigned char *nextKey);

/*
 * Function: DeriveSubKeys
 * -----------------------
 * Derives the different sub keys from the current seassion key.
 */
int DeriveSubKeys(unsigned char masterSessionkey[KEY_SIZE], unsigned char encKey[KEY_SIZE], unsigned char drnKey[KEY_SIZE], unsigned char tagKey[KEY_SIZE], unsigned char idKey[KEY_SIZE]);
/*
 * Funtion: GenerateMasterKey
 * --------------------------
 * Generates a new truely random bit string.
 */
int GenerateMasterKey(unsigned char *masterKey);

/*
 * Funtion: GenerateIV
 * --------------------------
 * Generates a new truely random bit string.
 */
int GenerateIV(unsigned char *iv);

/*
 * Function: PRF
 * -------------
 * Pseudo Random Function, based on CMAC (AES_256) and stretches the output via PRG_128.
 *
 * input: input of the PRF.
 * inputSize: size of the input.
 * key: 256 bit large key of the PRF.
 * output: buffer holdung the PRF result.
 * outputSize: the requested amount of output bytes (max 256).
 *
 * returns: 0 on failure and 1 on success.
 */
int PRF(unsigned char *input, size_t inputSize, unsigned char *key, unsigned char *output, uint8_t outputSize);

// Random things:

/*
 * Function: UniformRandomInt
 * --------------------------
 * Generates a uniformly distributed random number, within a upper bound. Inclusive 0 exclusive the upper bound.
 * [0, upperBound)
 * The random number is based on AES based PRG.
 *
 * ctx: Context of the underlying PRG.
 * upperBound: upper bound.
 *
 * returns: the uniformly distributed random number.
 */
unsigned int UniformRandomInt(PRGContext *ctx, const unsigned int upperBound);

/*
 * Funtion: DRN
 * ------------
 * Deterministic Distinct Random Number Generator.
 * Generates a list of k distinct random numbers. [0, upperBound) Inclusive 0 exclusive the upper bound.
 * seed: a secure 256 bit large seed
 * k: the number of distinct random numbers withing the given bound.
 * upperBound: the upper bound.
 * kRandom: array holding the k distinct random numbers, of size k.
 *
 * returns: 0 on failure and 1 on success.
 */
int DRN(unsigned char *seed, int k, const int upperBound, int *kRandom);

// AES stuff:

/*
 * Function: AES_256_CTR_encrypt
 * AES_256_CTR encryption, without padding (!), the plaintext has to be a multiple of the AES block legth (16 byte).
 *
 * plaintext: plaintext.
 * plaintextSize: plaintext size, has to be a multiple of the AES block length.
 * key: encryption key.
 * iv: Initialization Vector 16 byte!.
 * ciphertextBuffer: ciphertext buffer, with the same size of the plaintext.
 *
 * returns: the ciphertext length.
 */
int AES_256_CTR_encrypt(unsigned char *plaintext, int plaintextSize, unsigned char *key, unsigned char *iv, unsigned char *ciphertextBuffer);

/*
 * Function: AES_256_CTR_decrypt
 * AES_256_CTR decryption, without padding (!), the ciphertext has to be a multiple of the AES block legth (16 byte).
 *
 * ciphertextBuffer: ciphertext buffer, with the same size of the plaintext.
 * ciphertextSize: ciphertext size, has to be a multiple of the AES block length.
 * key: decryption key.
 * iv: Initialization Vector 16 byte!.
 * plaintext: plaintext.
 *
 * returns: the plaintext length.
 */
int AES_256_CTR_decrypt(unsigned char *ciphertext, int ciphertextSize, unsigned char *key, unsigned char *iv, unsigned char *plaintextBuffer);

/*
 * Funtion: CMAC
 * -------------
 * AES_256_CBC based MAC.
 *
 * key: MAC key.
 * input: message.
 * inputSize: message size.
 * output: the MAC.
 * outputSize: will return 16.
 * maxOutputSize: the size of the output buffer should be larger than 16 bytes.
 *
 * returns: 0 on failure and 1 on success.
 */
int CMAC(unsigned char *key, unsigned char *input, size_t inputSize, unsigned char *output, size_t *outputSize, size_t maxOutputSize);

#endif /* Crypto_h */
