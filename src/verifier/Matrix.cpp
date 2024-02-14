//
//  Matrix.cpp
//  verifier
//
//  Created by Florian on 17.11.23.
//

#include "Matrix.hpp"
#include <math.h>
#include <algorithm>
#include <iostream>
#include <dispatch/dispatch.h>
#include "RandomStuff.hpp"

namespace Matrix {
    /*
     Creates a new Matrix
     */
    MatrixType *Create(size_t m, size_t n)
    {
        size_t rows = m;
        size_t cols = n;
        
        MatrixType *matrix = new MatrixType;
        matrix->rows = rows;
        matrix->colsInBits = cols;
        size_t int32ColCount = ceil(cols / static_cast<float>(B_BITS));
        matrix->buckets = int32ColCount;
        
        matrix->data = new unsigned int [rows * int32ColCount]();
        
        return matrix;
    }
    
    void MatrixType::Print()
    {
        // print the matrix
        for (unsigned int i = 0; i < rows; ++i) {
            for (unsigned int j = 0; j < colsInBits; ++j) {
                std::cout << (int)(*this)(i, j);
                /*for (int b = B_BITS-1; b >= (ceil(matrix.colsInBits/B_BITS) == j ? B_BITS - (matrix.colsInBits%B_BITS):0); --b) {
                    //int bit = (matrix.data[i * matrix.buckets + j] >> b) & 1;
                    int bit = (int)matrix(i, j);
                    std::cout << (int)matrix(i, j);
                }*/
                std::cout << ' ';// << matrix.data[i * matrix.buckets + j] << ' ';
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    
    void MatrixType::swapRows(size_t l, size_t k)
    {
        // Swap the rows by swapping the corresponding elements in the data vector
        for (size_t i = 0; i < buckets; ++i) {
            std::swap(data[l * buckets + i], data[k * buckets + i]);
        }
        
    }
    
    
    void BMatrixType::Print() {
        for (int row = 0; row < this->rows; ++row) {
            for (int col = 0; col < this->colsInBits; ++col) {
                std::cout << (int)(*this)(row, col) << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl;
    }
    
    void BMatrixType::swapRows(int l, int k) {
        // Swap the rows by swapping the corresponding elements in the data vector
        for (size_t i = 0; i < buckets; ++i) {
            std::swap(data[l * buckets + i], data[k * buckets + i]);
        }
    }
    // if used data has to be freed externaly.
    void BMatrixType::SetCustomDataPointer(B256 *data){
        if (this->freeableData) {
            delete [] this->data;
        }
        
        this->data = data;
        this->freeableData = false;
    }
    
    // if used data has to be freed externaly.
    void MatrixType::SetCustomDataPointer(unsigned int *data){
        if (this->freeableData) {
            delete [] this->data;
        }
        
        this->data = data;
        this->freeableData = false;
    }
    
    // Destructor
    MatrixType::~MatrixType() {
        if (freeableData && data != nullptr) {
            delete[] data;
            data = nullptr;
        }
    }
    
    MatrixType *I(size_t size) {
        MatrixType *I = Create(size, size);
        for (size_t i = 0; i < size; ++i) {
            (*I).toggle(i, i);
        }
        
        return I;
    }
        
    void FillWithRandomness(MatrixType *m, int k)
    {
        for (unsigned int c = 0; c < m->colsInBits; c++)
        {
            size_t *randoms = distinctRandomEz(((unsigned int)m->rows - 1), k);
            // Iterate through the column c
            for (int i = 0; i < k; ++i) {
                size_t row = randoms[i];
                // unsigned int bitmask = (1 << (B_BITS - (c % B_BITS + 1)));
                // set this to a 1
                m->toggle(row, c);// data[row * (m->buckets) + (c / B_BITS)] ^= bitmask;
            }
            delete [] randoms;
        }
    }
}
