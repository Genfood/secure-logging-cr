//
//  Matrix.hpp
//  verifier
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 17.11.23.
//

#ifndef Matrix_hpp
#define Matrix_hpp

#define BITS_PER_BYTE 8
#define BYTES 4 // 4 for unsigned int
#define B_BITS (BITS_PER_BYTE * BYTES)
#define B_B_BITS 256

#include <algorithm>
#include <iostream>

namespace Matrix {
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
                    // Handle out-of-range member index
                    std::cout << "Invalid member index" << std::endl;
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
            result.x = x ^ other.x;
            result.y = y ^ other.y;
            result.z = z ^ other.z;
            result.w = w ^ other.w;
            return result;
        }

    };
    
    struct BMatrixType {
        B256 *data;
        int rows;
        int buckets;
        int colsInBits;
        
        // Get
        bool operator()(const int row, const int col) const {
            return data[row * buckets + (col / B_B_BITS )].getBit(col%B_B_BITS);
        }
        void SetCustomDataPointer(B256 *data);
        void swapRows(int l, int k);
        void Print();
        BMatrixType(int m, int n): rows(m), colsInBits(n){
            int int32ColCount = ceil(colsInBits / static_cast<float>(B_B_BITS));
            buckets = int32ColCount;
            data = new B256[rows * int32ColCount]();
        }
        ~BMatrixType(){
            if (this->freeableData) {
                delete [] this->data;
            }
        }
        
        static BMatrixType* I(int size) {
            BMatrixType* mat = new BMatrixType(size, size);
            for (int i = 0; i < size; ++i) {
                mat->setBit(i, i);
            }
            return mat;
        }
        
        void setBit(const int row, const int col) {
            data[row * (buckets) + (col / B_B_BITS)].setBit(col%B_B_BITS);
        }
        
    private:
        bool freeableData;
    };
    
    
    struct MatrixType {
        unsigned int *data;
        size_t rows;
        size_t buckets;
        size_t colsInBits;
        void SetCustomDataPointer(unsigned int *data);
        
        // Get
        bool operator()(const size_t row, const size_t col) const {
            size_t i = row * buckets + col / B_BITS;
            size_t offset = B_BITS - 1 - (col % B_BITS);
            return ( data[i] >> offset) & 1;
        }
        
        void swapRows(size_t l, size_t k);
        void Print();
        ~MatrixType();
        
        void toggle(const size_t row, const size_t col) {
            size_t i = row * buckets + col / B_BITS;
            size_t offset = B_BITS - 1 - (col % B_BITS);
            data[i] ^= (1u << offset);
        }
    private:
        bool freeableData = true;
    };
    
    MatrixType *Create(size_t m, size_t n);
    
    MatrixType *I(size_t size);
    void FillWithRandomness(MatrixType *m, int k = 5);
    
    
}

#endif /* Matrix_hpp */
