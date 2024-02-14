//
//  PIShared.h
//  shared
//
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


int CreateID(unsigned char *key, int j, unsigned char IDlj[ID_LEN]);
int CreateIntegrityTag(unsigned char *key, unsigned char *XORlj, unsigned char Tlj[INTEGRITY_TAG_LEN]);

// utility functions
unsigned char *XOR(unsigned char *buffer1, unsigned char *buffer2, int size);
void printInHex(unsigned char *out, int len);

#endif /* PIShared_h */
