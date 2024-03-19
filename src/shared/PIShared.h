//
//  PIShared.h
//  shared
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 15.11.23.
//

#ifndef PIShared_h
#define PIShared_h

#define MESSAGE_LEN 1024 // The max len of an log entry message.
#define INTEGRITY_TAG_LEN 16 // The integrity tag len.
#define ID_LEN 16 // The ID (to find the correct key for this log entry) len.
#define MAC_LEN 16 // Encrypt then MAC HMAC len. ??CMAC??
#define CIPHERTEXT_LEN (IV_SIZE + MESSAGE_LEN + MAC_LEN)
#define LOG_LEN (CIPHERTEXT_LEN + INTEGRITY_TAG_LEN + ID_LEN)
#define LOG_EXTENSION ".log.enc"
#define KEY_EXTENSION ".key"
#define K 5
#define C 1.1244

/*
 * Function: CreateID
 * ------------------
 * ID_{l_j} = PRF_{K_i} (\gamma ' || j)
 */
int CreateID(unsigned char *key, int j, unsigned char IDlj[ID_LEN]);

/*
 * Function: CreateIntegrityTag
 * ----------------------------
 * T_{l_j} = PRF_{K_i} (XOR_{l_j})
 */
int CreateIntegrityTag(unsigned char *key, unsigned char *XORlj, unsigned char Tlj[INTEGRITY_TAG_LEN]);

// utility functions
void printInHex(unsigned char *out, int len);

#endif /* PIShared_h */
