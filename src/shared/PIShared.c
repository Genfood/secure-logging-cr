//
//  PIShared.c
//  shared
//
//  Created by Florian on 15.11.23.
//

#include "PIShared.h"
#include <stdlib.h>
#include <string.h>
#include "Crypto.h"

int CreateID(unsigned char *key, int j, unsigned char IDlj[ID_LEN])
{
    int inputSize = ID_LEN + sizeof(int);
    unsigned char inputBuffer[inputSize];
    
    memcpy(inputBuffer, GAMMA_DASH, AES_BLOCK_LEN);
    memcpy(inputBuffer + AES_BLOCK_LEN, &j, sizeof(int));
    
    if (0 == PRF(inputBuffer, inputSize, key, IDlj, ID_LEN)) {
        perror("Error: Failed to create the ID for the key + j.\n");
        return 0;
    }
    
    return 1;
}

int CreateIntegrityTag(unsigned char *key, unsigned char *XORlj, unsigned char Tlj[INTEGRITY_TAG_LEN])
{
    if (0 == PRF(XORlj, CIPHERTEXT_LEN, key, Tlj, INTEGRITY_TAG_LEN)) {
        perror("Error: Failed to create the integrity tag.\n");
        
        return 0;
    }
    
    return 1;
}

void printInHex(unsigned char *out, int len)
{
    for (int i = 0; i < len; ++i) {
        printf("%02X ", out[i]);
    }
    printf("\n");
}
