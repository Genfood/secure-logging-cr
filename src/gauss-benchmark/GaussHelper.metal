//
//  GaussHelpers.metal
//  gauss-benchmark
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 18.10.23.
//

#include <metal_stdlib>
#include <metal_math>

//#define B_BITS 64
#define V_BITS 256 // für vektoren


#define GET_POS(col, BBITS) ((BBITS) - ((col) % (BBITS)))
//#define GET_BUCKET(col) ((col) / B_BITS)

#define IS_ONE(var,pos) ((var) & (1ul<<(pos)))
//#include "Matrix.h"



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

kernel void partial_pivoting_1(device const uint *m [[buffer(0)]],
                                      device const int *current_row [[buffer(1)]],
                                      device const int *current_col [[buffer(2)]],
                                      device const int *buckets [[buffer(3)]],
                                      device int &index [[buffer(4)]],
                                      uint i [[thread_position_in_grid]])
{
    // +1 because the current row is not affected.
    int row = (*current_row) + i + 1;
    if (1 == m[row * (*buckets) + *current_col])
    {
        // index is the row
        index = row;
    }
}

kernel void partial_pivoting_32(device const uint *m [[buffer(0)]],
                                      device const int *current_row [[buffer(1)]],
                                      device const int *current_col [[buffer(2)]],
                                      device const int *buckets [[buffer(3)]],
                                      device int &index [[buffer(4)]],
                                      uint i [[thread_position_in_grid]])
{
    int posInBucket = GET_POS(*current_col, 31); // always -1
        
    // +1 because the current row is not affected.
    int row = (*current_row) + i + 1;
    if (IS_ONE(m[row * (*buckets) + (*current_col/32)], posInBucket))
    {
        // index is the row
        index = row;
    }
}

kernel void partial_pivoting_64(device const ulong *m [[buffer(0)]],
                                      device const int *current_row [[buffer(1)]],
                                      device const int *current_col [[buffer(2)]],
                                      device const int *buckets [[buffer(3)]],
                                      device int &index [[buffer(4)]],
                                      uint i [[thread_position_in_grid]])
{
    int posInBucket = GET_POS(*current_col, 63); // always -1
        
    // +1 because the current row is not affected.
    int row = (*current_row) + i + 1;
    if (IS_ONE(m[row * (*buckets) + (*current_col/64)], posInBucket))
    {
        // index is the row
        index = row;
    }
}

kernel void partial_pivoting_256(device const ulong4 *m [[buffer(0)]],
                                 device const int *current_row [[buffer(1)]],
                                 device const int *current_col [[buffer(2)]],
                                 device const int *buckets [[buffer(3)]],
                                 device int &index [[buffer(4)]],
                                 uint i [[thread_position_in_grid]],
                                 constant uint& threadCount[[buffer(5)]])
{
    // check bounderies.
    if(i >= threadCount)
        return;
    
    int pos = *current_col % 256;
    
    // +1 because the current row is not affected.
    int row = (*current_row) + i + 1;
    
    ulong4 m_value = m[row * (*buckets) + (*current_col / 256)];//GET_BUCKET(*current_col)];
    
    if (isOne(m_value, pos)) {
        // index is the row
        index = row;
    }
}



kernel void xor_row_1(device uint *m [[buffer(0)]],
                      device const int *current_row [[buffer(1)]],
                      device const int *current_col [[buffer(2)]], // renmame to current bucket
                      device const int *current_bucket [[buffer(3)]],
                      device const int *buckets [[buffer(4)]],
                      constant uint& threadCount[[buffer(5)]],
                      uint i [[thread_position_in_grid]])
{
    // bounderies check
    if (i >= threadCount)
        return;
    
    int row = (*current_row) + i +1;
    
    if (1 == m[row * (*buckets) + *current_col])
    {
        int currentRowStartIndex = (*current_row) * (*buckets);
        int rowStartIndex = row * (*buckets);
        // XOR the rows starting from the specified column
        for (int c = *current_col; c < (*buckets); ++c) {
            int index = rowStartIndex + c;
            m[index] ^= m[currentRowStartIndex + c];
        }
    }
}

kernel void xor_row_32(device uint *m [[buffer(0)]],
                       device const int *current_row [[buffer(1)]],
                       device const int *current_col [[buffer(2)]], // renmame to current bucket
                       device const int *current_bucket [[buffer(3)]],
                       device const int *buckets [[buffer(4)]],
                       constant uint& threadCount[[buffer(5)]],
                       uint i [[thread_position_in_grid]])
{
    // bounderies check
    if (i >= threadCount)
        return;
    
    int row = (*current_row) + i +1;
    int posInBucket = GET_POS(*current_col, 31);
    
    
    int rowStartIndex = row * (*buckets);
    int currentRowStartIndex = (*current_row) * (*buckets);
    
    if (IS_ONE(m[rowStartIndex + *current_bucket], posInBucket))
    {
        // XOR the rows starting from the specified column
        for (int c = *current_bucket; c < (*buckets); ++c) {
            int index = rowStartIndex + c;
            m[index] ^= m[currentRowStartIndex + c];
        }
    }
}

kernel void xor_row_64(device ulong *m [[buffer(0)]],
                       device const int *current_row [[buffer(1)]],
                       device const int *current_col [[buffer(2)]], // renmame to current bucket
                       device const int *current_bucket [[buffer(3)]],
                       device const int *buckets [[buffer(4)]],
                       constant uint& threadCount[[buffer(5)]],
                       uint i [[thread_position_in_grid]])
{
    // bounderies check
    if (i >= threadCount)
        return;
    
    int row = (*current_row) + i +1;
    int posInBucket = GET_POS(*current_col, 63);
    int rowStartIndex = row * (*buckets);
    int currentRowStartIndex = (*current_row) * (*buckets);
    
    if (IS_ONE(m[rowStartIndex + *current_bucket], posInBucket))
    {
        // XOR the rows starting from the specified column
        for (int c = *current_bucket; c < (*buckets); ++c) {
            int index = rowStartIndex + c;
            m[index] ^= m[currentRowStartIndex + c];
        }
    }
}

kernel void xor_row_256(device ulong4 *m [[buffer(0)]],
                        device const int *current_row [[buffer(1)]],
                        device const int *current_col [[buffer(2)]], // renmame to current bucket
                        device const int *current_bucket [[buffer(3)]],
                        device const int *buckets [[buffer(4)]],
                        constant uint& threadCount[[buffer(5)]],
                        uint i [[thread_position_in_grid]])
{
    // bounderies check
    if (i >= threadCount)
        return;
    
    int row = (*current_row) + i +1;
    int pos = *current_col % 256;
    int baseIndex = row * (*buckets);
    
    ulong4 m_value = m[baseIndex + *current_bucket];
    
    if (isOne(m_value, pos)) {
        // XOR the rows starting from the specified column
        int rowBaseIndex = (*current_row) * (*buckets);

        for (int c = *current_bucket; c < (*buckets); ++c) {
            int index = baseIndex + c;
            m[index] ^= m[rowBaseIndex + c];
        }
    }
}

/*
kernel void xor_row_256_2D(device ulong4 *m [[buffer(0)]],
                        device const int *pivot_row [[buffer(1)]],
                        device const int *current_col [[buffer(2)]], // renmame to current bucket
                        device const int *buckets [[buffer(3)]],
                        uint2 gid [[thread_position_in_grid]],
                        constant uint &threadCount[[buffer(4)]])
{
    // bounderies check
    if (gid.x >= threadCount && gid.y >= *buckets)
        return;
    
    int row = (*pivot_row) + gid.x +1;
    //int current_bucket = gid.y;
    
    int pos = *current_col % 256;//GET_POS(*current_col); // # (B_BITS - 1 - ((col) % B_BITS))
    //int bucketIndex = *current_col / 256;
    int current_bucket = *current_col / 256;
    int baseIndex = row * (*buckets);
    
    ulong4 m_value = m[baseIndex + current_bucket];
    
    if (isOne(m_value, pos)) {
        // XOR the rows starting from the specified column
        int index = baseIndex + gid.y;
        int pivotRowBaseIndex = (*pivot_row) * (*buckets);
        
        m[index] = m[index] ^ m[pivotRowBaseIndex + gid.y];
    }
}
*/

