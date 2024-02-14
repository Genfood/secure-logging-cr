//
//  GaussianElimination_Helper.metal
//  verifier
//
//  Created by Florian on 21.11.23.
//

#include <metal_stdlib>

using namespace metal;

#define MESSAGE_LEN 1024 // The max len of an log entry message.#define AES_BLOCK_LEN 16
#define IV_SIZE 16
#define MAC_LEN 16 // Encrypt then MAC HMAC len. ??CMAC??

#define CIPHERTEXT_LEN 33//((IV_SIZE + MESSAGE_LEN + MAC_LEN) / (4*8)) // chars to long4 or (16+16+1024)/(4*8)

#define GET_BUCKET(col) ((col) / B_BITS)

#define B_BITS 256

using namespace metal;


// Check if the nth bit is set in the vector
bool isOne(ulong4 val, uint n) {
    // get the pos of the bit in the element
    ulong mask = 1ul << (63 - n%64);
    // check in which element the nth bit is.
    switch (n / 64) {
        case 0:
            return (val.x & mask) != 0;
        case 1:
            return (val.y & mask) != 0;
        case 2:
            return (val.z & mask) != 0;
        case 3:
            return (val.w & mask) != 0;
        default:
            return false;
    }
}

kernel void partial_pivoting(device const ulong4 *m [[buffer(0)]],
                             device const int *current_row [[buffer(1)]],
                             device const int *current_col [[buffer(2)]],
                             device const int *buckets [[buffer(3)]],
                             device int &index [[buffer(4)]],
                             constant uint& threadCount[[buffer(5)]],
                             uint i [[thread_position_in_grid]])
{
    // bounderies check
    if (i >= threadCount)
        return;
    
    // +1 because the current row is not affected.
    int row = (*current_row) + i + 1;
    
    if(isOne(m[row * (*buckets) + GET_BUCKET(*current_col)], *current_col % B_BITS))
    {
        // index is the row
        index = row;
    }
}

kernel void xor_row(device ulong4 *m [[buffer(0)]],
                    device ulong4 *I [[buffer(1)]],
                    device const int *current_row [[buffer(2)]],
                    device const int *current_col [[buffer(3)]],
                    device const int *current_bucket [[buffer(4)]],
                    device const int *buckets [[buffer(5)]],
                    device const int *bucketsI [[buffer(6)]],
                    constant uint& threadCount[[buffer(7)]],
                    uint i [[thread_position_in_grid]])
{
    
    // bounderies check
    if (i >= threadCount)
        return;
    
    // +1 because the current row doeas not need a xor treatment
    int row = (*current_row) + i +1;
    int rowBaseIndex = row * (*buckets);
    
    if(isOne(m[rowBaseIndex + *current_bucket], *current_col % B_BITS))
    {
        int currentRowStartIndex = (*current_row) * (*buckets);
        
        // XOR the rows starting from the specified column
        for (int c = *current_bucket; c < (*buckets); ++c) {
            int index = rowBaseIndex + c;
            m[index] ^= m[currentRowStartIndex + c];
        }
        
        int currentRowStartIndexI = (*current_row) * (*bucketsI);
        int rowBaseIndexI = row * (*bucketsI);
        for (int c = 0; c < (*bucketsI); ++c) {
            int index = index = rowBaseIndexI + c;
            I[index] ^= I[currentRowStartIndexI + c];
        }
    }
}

kernel void bookkeeping(device const ulong4 *I [[buffer(0)]],
                        device const int *size [[buffer(1)]],
                        device const int *buckets [[buffer(2)]],
                        device ulong4 *xors [[buffer(3)]],
                        device ulong4 *v [[buffer(4)]],
                        constant uint& threadCount[[buffer(5)]],
                        uint i [[thread_position_in_grid]])
{
    // bounderies check
    if (i >= threadCount)
        return;
    
    int rowInI = i * (*buckets);
    int elemInV = i * CIPHERTEXT_LEN;
    // col in I
    for (int col = 0; col < (*size); ++col) {
        
        // check if at the col in the row i is a 1
        if (isOne(I[rowInI + GET_BUCKET(col)], col % B_BITS)) {
                                            // It there is a 1 at the position of col, then get the element in the vector at the position of col and XOR it.
            int elemInXOR = col * CIPHERTEXT_LEN;
            // if so XOR it onto the ciphertext vector
            for (int j = 0; j < CIPHERTEXT_LEN; ++j) {
                v[elemInV + j] ^= xors[elemInXOR + j];
            }
        }
    }
}
