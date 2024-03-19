//
//  PlainGaussHelper.cpp
//  verifier
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 15.01.24.
//

#include "PlainGaussHelper.hpp"
#include <chrono>

using std::chrono::high_resolution_clock;
using std::chrono::duration;
using namespace std;

namespace PlainGaussHelper {
    void print(BMatrixType &M, bool debug);
    
    std::vector<PI::XOR_TYPE> Solve(BMatrixType *M, std::vector<PI::XOR_TYPE> &v, bool debug) {
        BMatrixType *I = BMatrixType::I(M->rows);
        
        print(*M, debug);
        auto t1 = high_resolution_clock::now();
        ForwardReduction(M, I);
        auto t2 = high_resolution_clock::now();
        duration<long, std::nano> ns_double = t2 - t1;
        
        cout << "Gaussian Elimination (Forward Reduction): " << ns_double.count() << " ns." << endl;
        
        print(*M, debug);
        
        std::cout << "Detected Rank: " << std::to_string(RankOf(*M)) << std::endl;
        
        auto t3 = high_resolution_clock::now();
        std::vector<PI::XOR_TYPE> _v = ApplyBookkeeping(*I, v);
        auto t4 = high_resolution_clock::now();
        duration<long, std::nano> ns_double_book = t4 - t3;
        
        cout << "Gaussian Elimination (Bookkeeping): " << ns_double_book.count() << " ns." << endl;
        delete I;
        
        auto t5 = high_resolution_clock::now();
        std::vector<PI::XOR_TYPE> c = BackSubstitution(*M, _v);
        auto t6 = high_resolution_clock::now();
        duration<long, std::nano> ns_double_back = t6 - t5;
        
        cout << "Gaussian Elimination (Back Substitution): " << ns_double_back.count() << " ns." << endl;
        return c;
    }
    
    void ForwardReduction(BMatrixType *M, BMatrixType *I){
        int null_col_counter = 0;
        int current_row = 0;
        
        while ((current_row + null_col_counter) < M->colsInBits)
        {
            int current_col = current_row + null_col_counter;
            int new_pivot_index = Pivot(M, current_row, current_col);
            
            if (-1 == new_pivot_index) {
                null_col_counter++;
                continue; // stop current iteration and check the next col for 1s
            }
            
            if (new_pivot_index != current_row) {
                M->swapRows(current_row, new_pivot_index);
                I->swapRows(current_row, new_pivot_index);
            }
            
            for (int row = current_row + 1; row < M->rows; ++row) {
                
                // if 1
                if (!(*M)(row,current_col)) {
                    continue;
                }
                
                // XOR
                int current_bucket = current_row / B_B_BITS;
                
                for (int c = current_bucket; c < M->buckets; ++c) {
                    int index = row * M->buckets + c;
                    M->data[index] = M->data[index] ^ M->data[current_row * M->buckets + c];
                }
                
                for (int c = 0; c < I->buckets; ++c) {
                    int index = row * I->buckets + c;
                    I->data[index] = I->data[index] ^ I->data[current_row * I->buckets + c];
                }
            }
            
            current_row++;
        }
    }
    
    
    std::vector<PI::XOR_TYPE> ApplyBookkeeping(BMatrixType &I, std::vector<PI::XOR_TYPE> &v) {
        std::vector<PI::XOR_TYPE> _v(I.rows, PI::XOR_TYPE{});
        const size_t ulongSize = sizeof(unsigned long);
        const size_t ulongLen = CIPHERTEXT_LEN / ulongSize;
        
        for (int row = 0; row < I.rows; ++row) {
            
            for (int col = 0; col < I.colsInBits; ++col) {
                if (I(row, col)) {
                    auto* vRowAsUlong = reinterpret_cast<unsigned long*>(v[col].data());
                    auto* _vRowAsUlong = reinterpret_cast<unsigned long*>(_v[row].data());

                    // XOR
                    for (int i = 0; i < ulongLen; ++i) {
                        _vRowAsUlong[i] ^= vRowAsUlong[i];
                    }
                }
            }
        }
        
        return _v;
    }
    
    std::vector<PI::XOR_TYPE> BackSubstitution(BMatrixType &M, std::vector<PI::XOR_TYPE> &v) {
        std::vector<PI::XOR_TYPE> ci (M.colsInBits, PI::XOR_TYPE{});
        const size_t ulongSize = sizeof(unsigned long);
        const size_t ulongLen = CIPHERTEXT_LEN / ulongSize;
        
        
        // we skip the rows which contains no cs
        for (int r = 0; r < M.colsInBits; ++r) {
            int row = M.colsInBits -1 -r; // use main diagonal (start at the main diagonal)(-1 one because we are starting at index 0 and colsInBits is the a number start counting at 1)
            
                                // It is not needed to check further than the main diagonal, (the matrix is overdetermined that why we sometimes check over the main diagonal)
            
            
            // The first element should be the 1 in the main diagonal, this is the single new entry which is unknown, and has to be taken from the vector v.
            // in some cases it is possible that there is a 0 at the main diagonal, thats why we are looping till we found the first 1, starting at the main diagonal.
            
             // begin at the diagonal.
            int col;
            for (col = row; col < M.colsInBits; ++col) {
                if (M(row, col)) {
                    // Reinterpret as unsigned long arrays for XOR
                    auto* vColAsUlong = reinterpret_cast<unsigned long*>(v[col].data());
                    auto* ciRowAsUlong = reinterpret_cast<unsigned long*>(ci[row].data());

                    
                    for (int i = 0; i < ulongLen; ++i) {
                        ciRowAsUlong[i] ^= vColAsUlong[i]; // The first vector will be taken from the original v
                    }
                    col++;
                    break; // we found the first Vektor now get the rest out of th c_results
                }
            }
            
            // XORing it with the current state of c_i, will remove all the vectors, that have been combined with it.
            for (; col < M.colsInBits; ++col) {
                if (M(row, col)) { // Check if there is a 1 at this index, and if so XOR the vector at this index on c_i
                    // Reinterpret as unsigned long arrays for XOR
                    auto* ciColAsUlong = reinterpret_cast<unsigned long*>(ci[col].data());
                    auto* ciRowAsUlong = reinterpret_cast<unsigned long*>(ci[row].data());

                    
                    for (int i = 0; i < ulongLen; ++i) {
                        ciRowAsUlong[i] ^= ciColAsUlong[i]; // XOR with already know vectors, to unpeel the current.
                    }
                }
            }
        }
        
        return ci;
    }
    
    int RankOf(BMatrixType &m) {
        for (int r = m.rows; r > 0; --r) {
            for (int c = m.colsInBits; c >= r; --c) {
                if (m(r,c)) {
                    return r+1;
                }
            }
        }
        
        return  0;
    }
    
    void print(BMatrixType &M, bool debug){
        if (debug) {
            M.Print();
        }
    }
}
