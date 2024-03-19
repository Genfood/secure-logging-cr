//
//  Matrix.hpp
//  gauss-benchmark
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 18.10.23.
//

#pragma once
#include <iostream>
#include "RandomStuff.hpp"


//#define B_BITS 256//4 * 8 * 8
//#define B_BITS_256 256
//B_BITS#define BB (T) (sizeof(T) * 8)

struct B1 {
    int x;
    
    inline void setBit(int pos) {
        x = 1;
    }
    
    inline bool getBit(int pos) {
        return x;
    }
    
    inline B1 operator^(const B1& other) const {
        B1 result;
        result.x = this->x ^ other.x;
        return result;
    }
};

struct B256 {
    unsigned long x;
    unsigned long y;
    unsigned long z;
    unsigned long w;
    
    // Function to set a bit at a given position
    void setBit(int n) {
        switch (n / 64) {
            case 0:
                x |= (1ul << (63 - n));
                return;
            case 1:
                y |= (1ul << (127 - n));
                return;
            case 2:
                z |= (1ul << (191 - n));
                return;
            case 3:
                w |= (1ul << (255 - n));
                return;
                
            default:
                return;
        }
    }
    
    // Function to get a bit at a given position
    bool getBit(int position) const {

        int memberIndex = position / 64; // Each unsigned long has 64 bits

        const unsigned long* target = nullptr;
        switch (memberIndex) {
            case 0:
                target = &x;
                break;
            case 1:
                target = &y;
                break;
            case 2:
                target = &z;
                break;
            case 3:
                target = &w;
                break;
            default:
                // Handle out-of-range member index
                std::cout << "Invalid member index" << std::endl;
                return false; // Return an error value or handle the invalid case
        }

        int bitIndexInMember = 63 - position % 64;

        return ((*target >> bitIndexInMember) & 1) != 0;
    }
    
    B256 operator^(const B256& other) const {
        B256 result;
        result.x = this->x ^ other.x;
        result.y = this->y ^ other.y;
        result.z = this->z ^ other.z;
        result.w = this->w ^ other.w;
        return result;
    }
};

struct B32 {
    unsigned int x;
    
    void setBit(int pos) {
        x |= (1 << (31 - pos));
    }
    
    bool getBit(int pos) {
        return ((x >> (31 - (pos % 32))) & 1) != 0;
    }
    B32 operator^(const B32& other) const {
        B32 result;
        result.x = this->x ^ other.x;
        return result;
    }
    
};

struct B64 {
    unsigned long x;
    
    void setBit(int pos) {
        x |= (1ul << (63 - pos));
    }
    
    bool getBit(int pos) {
        return (x >> (63 - (pos % 64))) & 1;
    }
    B64 operator^(const B64& other) const {
        B64 result;
        result.x = this->x ^ other.x;
        return result;
    }
};

struct B16 {
    unsigned char x;
    unsigned char y;
    
    // Function to set a bit at a given position
    void setBit(int position) {

        if (position < 8) {
            // Set bit in 'x' (MSB part)
            x |= (1 << (7 - position)); // Flipping the position for MSB
        } else {
            // Set bit in 'y' (LSB part)
            y |= (1 << (15 - position)); // Flipping the position for LSB
        }
    }
    
    bool getBit(int position) {
        
        unsigned char* target = (position < 8) ? &x : &y;
        return ((*target >> (7 - (position % 8))) & 1) != 0;
    }
    B16 operator^(const B16& other) const {
        B16 result;
        result.x = this->x ^ other.x;
        return result;
    }
};


// 40sec
// 15 ulong4
template<typename T>
struct Matrix {
    T *data;
    int rows;
    int buckets;
    int colsInBits;
    bool freeableData = true;
    
    
    int bits;
    //template<typename T>
    void SetCustomDataPointer(T *data){
        if (this->freeableData) {
            delete [] this->data;
        }
        
        this->data = data;
        this->freeableData = false;
    }
    
    ~Matrix(){
        if (this->freeableData) {
            delete [] this->data;
        }
    }
    
    bool operator () (int row, int col) const {
        return data[row * buckets + (col / bits )].getBit(col%bits);
    }
    
    void toggle(int row, int col) {
        data[row * (buckets) + (col / bits)].setBit(col%bits);
    }
    
    void print()
    {
        // print the matrix
        for (unsigned int i = 0; i < rows; ++i) {
            for (int c = 0; c < colsInBits; ++c) {
                bool bit = (*this)(i, c);
                std::cout << bit;
            }
            /*
            //for (unsigned int j = 0; j < buckets; ++j) {
                
                for (int bi  = 0; bi < (ceil(colsInBits/bits) == j ? (colsInBits%bits) : bits); ++bi) {
                    
                    bool bit = (*this)(i, bi);// data[i * buckets + j].getBit(bi);
                    std::cout << bit;
                }
                std::cout << ' ';// << matrix.data[i * matrix.buckets + j] << ' ';
             */
            //}
            std::cout << std::endl;
        }
    }
};


template<typename T>
Matrix<T> Create(int rows, int cols, int bits) {
    Matrix<T> matrix;
    matrix.rows = rows;
    matrix.colsInBits = cols;
    matrix.bits = bits;
    matrix.buckets = ceil(cols / static_cast<float>(matrix.bits));
    matrix.data = new T[rows * matrix.buckets]{};
    return matrix;

}



template<typename T>
Matrix<T> CreateRandomMatrix(int m, int n, int k, int bits) {
    Matrix<T> matrix = Create<T>(m, n, bits);
    for (int c = 0; c < matrix.colsInBits; c++)
    {
        size_t *randoms = distinctRandomEz((matrix.rows - 1), k);
        // Iterate through the column c
        for (int i = 0; i < k; ++i) {
            int row = (int)randoms[i];
            //int pos = (B_BITS_256 - (c % B_BITS_256 + 1));
            // set this to a 1
            matrix.toggle(row, c);
            
        }
    }
    return matrix;
}
