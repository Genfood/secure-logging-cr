//
//  GaussianElimination.cpp
//  verifier
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 21.11.23.
//

#include "GaussianElimination.hpp"
#include "PlainGaussHelper.hpp"
#include <iostream>
#include <chrono>

using std::chrono::high_resolution_clock;
using std::chrono::duration;
using namespace std;

#define SIZE_OF_BUCKET sizeof(B256)
#define IS_ONE(var,pos) ((var) & (1<<(pos)))

namespace Gauss{
    GaussianElimination::GaussianElimination(BMatrixType *M, std::vector<PI::XOR_TYPE> v, bool debugPrints)
        :_m(*M), I(BMatrixType::I(M->rows)), _v(v), _debug(debugPrints)
    {
        // get GPU
        MTL::Device *device = MTL::CreateSystemDefaultDevice();
        
        // Create Metal Factory, this class handles all the metall realted code.
        _factory = new MetalFactory(device);
        
        // Create all necessary buffers, fill them with data
        prepareData();
        
        // set pointer of the shared matrix buffer:
        _m.SetCustomDataPointer((B256*)_mBuffer->contents()); // for my understanding this pointer never changes for the whole execution.
        I->SetCustomDataPointer((B256*)_iBuffer->contents());
        
    }
    
    void GaussianElimination::prepareData()
    {
        int mSize = _m.rows * _m.buckets * SIZE_OF_BUCKET;
        
        // Convert C++ std stuff into plain C
        size_t vSize = _v.size() * CIPHERTEXT_LEN;
        
        // Buffer that can be pre filled:
        _mBuffer            = _factory->newBuffer(_m.data, mSize);
        _iBuffer            = _factory->newBuffer(I->data, I->rows * I->buckets * SIZE_OF_BUCKET);
        
        _rowsBuffer         = _factory->newBuffer(&_m.rows, sizeof(int));
        _bucketsBuffer      = _factory->newBuffer(&_m.buckets, sizeof(int));
        _bucketsIBuffer        = _factory->newBuffer(&(I->buckets), sizeof(int));
        
        _xorsBuffer            = _factory->newBuffer(_v.data(), vSize); // holds the initial values of the vector v
        
        
        // Buffer that cant be pre filled, because they change for every call:
        _currentColBuffer   = _factory->newBuffer(sizeof(int));
        _currentBucketBuffer= _factory->newBuffer(sizeof(int));
        _currentRowBuffer   = _factory->newBuffer(sizeof(int));
        // or will be used for return values
        _indexResultBuffer  = _factory->newBuffer(sizeof(int));
        _cBuffer            = _factory->newBuffer(vSize);
        _vBuffer            = _factory->newBuffer(vSize); // holds the result after bookkepping
    }
    
    
    
    void GaussianElimination::sendXORComputeCommand(int &currentRow/*, int currentBucket*/)
    {
        // +1 because we dont need to do something with the current row, no xor here.
        int size = _m.rows - (currentRow +1);
        
        // TODO: at a specific threshold a diffrent solution might be better.
        if (1 > size) {
            return;
        }
        
        //MetalFactory::setBufferWithInt32(_currentRowBuffer, currentRow);
        //MetalFactory::setBufferWithInt32(_currentColBuffer, currentBucket);
        MTL::Buffer *buffers [] = {
            _mBuffer,
            _iBuffer,
            _currentRowBuffer,
            _currentColBuffer,
            _currentBucketBuffer,
            _bucketsBuffer,
            _bucketsIBuffer
        };
        
        _factory->sendCommand(_factory->XorPSO, buffers, 7, size, false);
    }
    
    int GaussianElimination::sendPartitialPivotComputeCommand(int currentRow, int currentCol)
    {
        int size = _m.rows - (currentRow +1);
        
        // TODO: at a specific threshold a diffrent solution might be better.
        if (1 > size) {
            return currentRow;
        }
        
        MetalFactory::setBufferWithInt32(_currentRowBuffer, currentRow);
        MetalFactory::setBufferWithInt32(_currentColBuffer, currentCol);
        MetalFactory::setBufferWithInt32(_indexResultBuffer, currentRow); // set as default the current row
        
        MTL::Buffer *buffers [] = {
            _mBuffer,
            _currentRowBuffer,
            _currentColBuffer,
            _bucketsBuffer,
            _indexResultBuffer
        };
        
        
        _factory->sendCommand(_factory->PivotPSO, buffers, 5, size, false);
        
        return *(int*)_indexResultBuffer->contents();
    }
    
    
    
    void GaussianElimination::gaussForwardReduction()
    {
        int null_col_counter = 0;
        int *current_row;
        int *current_col;
        int *current_bucket;
        
        // set pointer to buffer, to work on the same pointer than the GPU.
        current_row     = (int*)_currentRowBuffer->contents();
        current_col     = (int*)_currentColBuffer->contents();
        current_bucket     = (int*)_currentBucketBuffer->contents();
        
        
        while ((*current_row + null_col_counter) < _m.colsInBits)
        {
            *current_col = *current_row + null_col_counter;
            
            
            int new_pivot_index = PlainGaussHelper::Pivot(&_m, *current_row, *current_col);
            
            if (-1 == new_pivot_index) {
                null_col_counter++;
                continue; // stop current iteration and check the next col for 1s
            }
            
            if (new_pivot_index != (*current_row)) {
                println("INFO: Now swap rows!");
                _m.swapRows(*current_row, new_pivot_index);
                I->swapRows(*current_row, new_pivot_index); // bookkeeping
                
                println("INFO: Matrix after rows has been swapped:");
                printMatrixes();
            }
            
            *current_bucket = (*current_col)/B_B_BITS;
            sendXORComputeCommand(*current_row);
            
            println("INFO: Matrix after the xor:");
            printMatrixes();
            (*current_row)++;
        }
    }
    
    std::vector<PI::XOR_TYPE> GaussianElimination::sendBookkeepingCommand() {
        MTL::Buffer *buffers [] = {
            _iBuffer,
            _rowsBuffer, // the number of rows in M is eqvivalent to the size of the indentity Matrix I
            _bucketsIBuffer,
            _xorsBuffer,
            _vBuffer
        };
        
        
        //std::cout << (*(int*)_bucketsIBuffer->contents()) << std::endl;
        _factory->sendCommand(_factory->BookkeepingPSO, buffers, 5, I->rows, true);
        
        auto size = _v.size();
        std::vector<PI::XOR_TYPE> v(size, PI::XOR_TYPE{});
        
        // get the result of the bookkeeping kernel:
        unsigned char *vBuffer = (unsigned char *)_vBuffer->contents();
        
        for (int i = 0; i < size; ++i) {
            PI::XOR_TYPE ciphertext;
            // copy xor array
            for (int j = 0; j < CIPHERTEXT_LEN; ++j) {
                ciphertext[j] = vBuffer[i * CIPHERTEXT_LEN + j];
            }
            v[i] = ciphertext;
        }
        
        return v;
    }
    
    std::vector<PI::XOR_TYPE> GaussianElimination::solve()
    {
        // forward reduction
        auto t1 = high_resolution_clock::now();
        gaussForwardReduction();
        auto t2 = high_resolution_clock::now();
        
        duration<long, std::nano> ns_double = t2 - t1;
        
        cout << "Gaussian Elimination (Forward Reduction): " << ns_double.count() << " ns." << endl;
        
        println("M and T after FR:");
        std::cout << "Detected Rank: " << std::to_string(RankOf(_m)) << std::endl;
        printMatrixes();
        // apply bookkeeping
        auto t3 = high_resolution_clock::now();
        std::vector<PI::XOR_TYPE> v_afterBookkeeping = sendBookkeepingCommand();
        auto t4 = high_resolution_clock::now();
        
        duration<long, std::nano> ns_double_book = t4 - t3;
        
        cout << "Gaussian Elimination (Bookkeeping): " << ns_double_book.count() << " ns." << endl;
        
        println("M and T after BK:");
        printMatrixes();
        
        // Back Substitution
        auto t5 = high_resolution_clock::now();
        std::vector<PI::XOR_TYPE> c = PlainGaussHelper::BackSubstitution(_m, v_afterBookkeeping);
        auto t6 = high_resolution_clock::now();
        
        duration<long, std::nano> ns_double_back = t6 - t5;
        
        cout << "Gaussian Elimination (Back Substitution): " << ns_double_back.count() << " ns." << endl;
        
        return c;
    }
    
    void GaussianElimination::printMatrixes() {
#ifdef DEBUG
        if (!_debug) {
            return;
        }
        _m.Print();
        I->Print();
#endif
    }
    void GaussianElimination::println(std::string s)
    {
#ifdef DEBUG
        if (!_debug) {
            return;
        }
        
        std::cout << s <<std::endl;
#endif
    }
    
    GaussianElimination::~GaussianElimination()
    {
        // TDOO: release all pointers!!
        // others:
        delete _factory;
        delete I;
        
        // buffers:
        _mBuffer->release();
        _cBuffer->release();
        _xorsBuffer->release();
        _iBuffer->release();
        _bucketsIBuffer->release();
        _currentRowBuffer->release();
        _currentColBuffer->release();
        _rowsBuffer->release();
        _bucketsBuffer->release();
        _indexResultBuffer->release();
        _vBuffer->release();
    }
    
    int GaussianElimination::RankOf(BMatrixType &m) {
        for (int r = m.rows; r > 0; --r) {
            for (int c = m.colsInBits; c >= r; --c) {
                if (m(r,c)) {
                    return r+1;
                }
            }
        }
        
        return  0;
    }
}
