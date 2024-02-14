//
//  Crypto.c
//  shared
//
//  Created by Florian on 15.11.23.
//

#include "Crypto.h"
#include <math.h>
#include <string.h>
#include <limits.h>
#include "PIShared.h"

#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/cmac.h>
#include <openssl/rand.h>

#define CMAC_LEN 16

static int _PRG(unsigned char *seed, unsigned int *counter, const EVP_CIPHER *cipher, unsigned char *buffer, int size);
static int AES_encrypt(const EVP_CIPHER *cipher, unsigned char *plaintext, int plaintextSize, unsigned char *key, unsigned char *iv, unsigned char *ciphertextBuffer);
static int exists(int element, const int arr[], size_t size);
static void handleErrors(void);


PRG128Context *CreatePRG128Context(unsigned char seed[16])
{
    PRG128Context *context = malloc(sizeof(PRG128Context));
    
    if (context == NULL)
    {
        perror("Failed to allocate memory.");
        return NULL;
    }
        
    context->counter = 0;
    memcpy(context->seed, seed, 16);
    
    return context;
}
PRGContext *CreatePRGContext(unsigned char seed[KEY_SIZE])
{
    PRGContext *context = malloc(sizeof(PRGContext));
    
    if (context == NULL)
    {
        perror("Failed to allocate memory.");
        return NULL;
    }
        
    context->counter = 0;
    memcpy(context->seed, seed, KEY_SIZE);
    
    return context;
}

static int _PRG(unsigned char *seed, unsigned int *counter, const EVP_CIPHER *cipher, unsigned char *buffer, int size)
{
    // calculate the number of aes blocks and ceil if it is not a multiple of the AES block size.
    int blocks = ceil((double)size / AES_BLOCK_LEN);
    int paddedSize = blocks * AES_BLOCK_LEN;
    
    unsigned char *input = calloc(paddedSize, sizeof(unsigned char));
    unsigned char *output = calloc(paddedSize, sizeof(unsigned char));
    
    // Each AES block contain the next higher counter sequence.
    for (unsigned long i = 0; i < blocks; ++i) {
        unsigned char *destinationPtr = &input[i * AES_BLOCK_LEN];
        unsigned long ctr = i + *counter;

        // Copy the unsigned long value to the block in the input buffer
        memcpy(destinationPtr, &ctr, sizeof(unsigned long));
    }
    
    // save to global counter.
    *counter += blocks;
    
    int outputLen = AES_encrypt(cipher, input, paddedSize, seed, NULL, output);
    free(input);
    
    // Validate output buffer length
    if (outputLen != paddedSize) {
        free(output);
        return 0;
    }
    // Fill result buffer, but only with the requested amount of random data.
    memcpy(buffer, output, size);
    free(output);
    
    return 1;
}


int PRG128(PRG128Context *ctx, unsigned char *buffer, int size)
{
    return _PRG(ctx->seed, &ctx->counter, EVP_aes_128_ecb(), buffer, size);
}

int PRG(PRGContext *ctx, unsigned char *buffer, int size)
{
    return _PRG(ctx->seed, &ctx->counter, EVP_aes_256_ecb(), buffer, size);
}


int AES_256_CTR_encrypt(unsigned char *plaintext, int plaintextSize, unsigned char *key, unsigned char *iv, unsigned char *ciphertextBuffer)
{
    return AES_encrypt(EVP_aes_256_ctr(), plaintext, plaintextSize, key, iv, ciphertextBuffer);
}

// based on: https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption#Encrypting_the_message
int AES_256_CTR_decrypt(unsigned char *ciphertext, int ciphertextSize, unsigned char *key, unsigned char *iv, unsigned char *plaintextBuffer)
{
    EVP_CIPHER_CTX *ctx;

    int len = 0;

    int plaintextLen;
    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
            handleErrors();
    
    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_ctr(), NULL, key, iv))  /* Failed to initialize aes in ECB mode. */
        handleErrors();
    
    // Disable padding, the total amount of data encrypted or decrypted must then be a multiple of the block size or an error will occur.
    if(1 != EVP_CIPHER_CTX_set_padding(ctx, 0))
        handleErrors();
    
    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintextBuffer, &len, ciphertext, ciphertextSize)) /* Failed to decrypt plaintext. */
        handleErrors();
    plaintextLen = len;
    
    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintextBuffer + len, &len))
        handleErrors();
    plaintextLen += len;
    
    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintextLen;
}

// based on: https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption#Encrypting_the_message
static int AES_encrypt(const EVP_CIPHER *cipher, unsigned char *plaintext, int plaintextSize, unsigned char *key, unsigned char *iv, unsigned char *ciphertextBuffer)
{
    EVP_CIPHER_CTX *ctx; // Was das? Context?
    int len;
    int ciphertextLen;
    
    /* Create and initialise the context */
    if (!(ctx = EVP_CIPHER_CTX_new())) {
        handleErrors();
        return 0;
    }
    
    
    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if (1 != EVP_EncryptInit_ex(ctx, cipher, NULL, key, iv)){
        handleErrors();
        return 0;
    }
    
    // Disable padding, the total amount of data encrypted or decrypted must then be a multiple of the block size or an error will occur.
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    
    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertextBuffer, &len, plaintext, plaintextSize)){
        handleErrors();
        return 0;
    }
    ciphertextLen = len;
    
    /*
     * Finalise the encryption. Further ciphertext bytes may be written at this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertextBuffer + len, &len))
    {
        handleErrors();
        return 0;
    }
    ciphertextLen += len;
    
    
    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);
    
    return ciphertextLen;
}

static int exists(int element, const int arr[], size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        if (arr[i] == element) {
            return 1;
        }
    }
    return 0;
}

/*
 Generates a uniformly distributed random number, within a upper bound. Inclusive 0 exclusive the upper bound.
 [0, upperBound)
 */
unsigned int UniformRandomInt(PRGContext *ctx, const unsigned int upperBound)
{
    unsigned long multipleOfUpperBound;
    unsigned int rand;
    unsigned char *randomBuffer;
    
    if (upperBound < 2)
        return 0;
    
    // eliminate the modul bias
    // https://research.kudelskisecurity.com/2020/07/28/the-definitive-guide-to-modulo-bias-and-how-to-avoid-it/
    // https://github.com/jedisct1/libsodium/blob/master/src/libsodium/randombytes/randombytes.c#L145
    // min = (1U + ~upperBound) % upperBound;
    // https://github.com/openbsd/src/blob/master/lib/libc/crypt/arc4random_uniform.c
    // get the largest multiple which is less than the number of diffrent values taht can be represented by an unsigned int (2^32).
    multipleOfUpperBound = (1UL << 32) - ((1UL << 32) % upperBound); // in the case the upper bound is a multiple of 2^32, the condition below holds anyeay, because it has to be less than 2^32, so it will work with the largest possible unsigned int.
    
    randomBuffer = malloc(sizeof(unsigned int));
    
    for(;;) {
        if (1 != PRG(ctx, randomBuffer, sizeof(unsigned int)))
        {
            perror("ERROR: PRG failed.");
            exit(EXIT_FAILURE);
            
        }
        
        memcpy(&rand, randomBuffer, sizeof(unsigned int));
        
        if (rand < multipleOfUpperBound)
            break;
    }
    
    free(randomBuffer);
    
    return rand % upperBound;
}

/*
 Deterministic Distinct Random Number Generator.
 Generates a list of k distinct random numbers. [0, upperBound) Inclusive 0 exclusive the upper bound.
 */
int DRN(unsigned char *seed, int k, int upperBound, int *kRandom)
{
    unsigned int rand;
    int i = 0;
    
    if (2 >= upperBound && upperBound > INT_MAX)
        return 0;
    
    // Fill the array with -1
    // thats why the upper bound cant be larger than int_max
    for (int i = 0; i<k; ++i)
        kRandom[i] = -1;
    
    PRGContext *ctx = CreatePRGContext(seed);
    
    while (i < k) {
        rand = UniformRandomInt(ctx, upperBound);
        
        // check if the random number already exists in the arra of k random numbers.
        if (!exists(rand, kRandom, k)){
            kRandom[i] = rand;
            i++;
        }
    }
    
    return 1;
}

/*
 Two phases PRF:
 1. CMAC, which returns a 128bit MAC tag which will than be used as key for a
 2. AES-128-ECB encryption of a counter.
 */
int PRF(unsigned char *input, size_t inputSize, unsigned char *key, unsigned char *output, uint8_t outputSize)
{
    size_t outputLenCMAC;
    
    // add a byte for the outputSize, max len for the outputSize is 2^8 = 128
    unsigned char _input[inputSize + 1], seed[16];
    
    memcpy(_input, input, inputSize);
    
    // The output size is an input value of the PRF, and should therefore change the output of the PRF the same way, as the key or input, would do.
    // input || outputSize, set the last byte to the output size.
    _input[inputSize] = outputSize;
    
    if(!CMAC(key, _input, inputSize, seed, &outputLenCMAC, CMAC_LEN)){
        perror("Failed to create CMAC as seed for a PRG as output for the variable PRF.");
        
        return 0;
    }
    
    // stretch or cut the output of the PRF, by applying a PRG.
    PRG128Context *ctx = CreatePRG128Context(seed);
    if(!PRG128(ctx, output, outputSize))
    {
        perror("Failed to create PRF output, when using PRG.");
        free(ctx);
        return 0;
    }
    
    free(ctx);
    
    return 1;
}

int KeyEvolution(unsigned char *key, unsigned char *nextKey)
{
    return PRF(GAMMA, 32, key, nextKey, KEY_SIZE);
}

int DeriveSubKeys(unsigned char masterSessionkey[KEY_SIZE], unsigned char encKey[KEY_SIZE], unsigned char drnKey[KEY_SIZE], unsigned char tagKey[KEY_SIZE], unsigned char idKey[KEY_SIZE])
{
    PRGContext *ctx = CreatePRGContext(masterSessionkey);
    
    unsigned char *output = calloc(4 * KEY_SIZE, 1);
    
    if (PRG(ctx, output, 4 * KEY_SIZE) != 1){
        perror("Failed to derive sub keys.");
        return 0;
    }
    memcpy(encKey, output, KEY_SIZE);
    memcpy(drnKey, output + KEY_SIZE, KEY_SIZE);
    memcpy(tagKey, output + (2 * KEY_SIZE), KEY_SIZE);
    memcpy(idKey, output + (3 * KEY_SIZE), KEY_SIZE);
    
    free(ctx);
    free(output);
    return 1;
}

int GenerateMasterKey(unsigned char *masterKey)
{
    return RAND_bytes(masterKey, KEY_SIZE);
}

int GenerateIV(unsigned char *iv)
{
    return RAND_bytes(iv, IV_SIZE);
}

// TODO: set the max output size inside the cmac function? Fixed to 16.
int CMAC(unsigned char *key, unsigned char *input, size_t inputSize, unsigned char *output, size_t *outputSize, size_t maxOutputSize/* Prevent buffer overflows, in the case that the maximal possible outbut buffer size is smaler than the actual output buffer. */)
{
    EVP_MAC *mac = EVP_MAC_fetch(NULL, "CMAC", NULL);
    if (mac == NULL){
        perror("Failed to fetch CMAC.");
        return 0;
    }
    
    EVP_MAC_CTX *ctx = EVP_MAC_CTX_new(mac);
    
    if (!ctx) {
        perror("Failed to create MAC contxt.");
        return 0;
    }
    
    // Sets the name of the underlying cipher to be used. The mode of the cipher must be CBC.
    // https://www.openssl.org/docs/man3.1/man7/EVP_MAC-CMAC.html
    OSSL_PARAM params[2];
    params[0] = OSSL_PARAM_construct_utf8_string("cipher", "aes-256-cbc", 0);
    params[1] = OSSL_PARAM_construct_end();
    
    // braucht einen Array, nicht nur ein pointer auf einen Parameter.
    if (EVP_MAC_CTX_set_params(ctx, params) != 1) {
        perror("Failed to set parameter.");
        // free
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return 0;
    }
    
    if (EVP_MAC_init(ctx, key, KEY_SIZE, NULL) != 1) {
        perror("Failed to init CMAC.");
        // free
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return 0;
    }
    
    if (EVP_MAC_update(ctx, input, inputSize) != 1) {
        perror("Failed to update CMAC.");
        // free
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return 0;
    }
    
    // If the maxOutputSize is to small, to hold the output -> the mission will be aborted.
    if (EVP_MAC_final(ctx, output, outputSize, maxOutputSize) != 1) {
        perror("Failed to create CMAC.");
        // free
        EVP_MAC_CTX_free(ctx);
        EVP_MAC_free(mac);
        return 0;
    }
    
    // free
    EVP_MAC_CTX_free(ctx);
    EVP_MAC_free(mac);
    return 1;
}

void handleErrors(void)
{
    ERR_print_errors_fp(stderr);
    abort();
}
