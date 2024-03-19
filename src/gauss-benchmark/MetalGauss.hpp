//
//  MetalGauss.hpp
//  gauss-benchmark
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 29.09.23.
//

#pragma once

#import <Foundation/Foundation.hpp>
#import <Metal/Metal.hpp>
#include <iostream>
#include <algorithm>
#include "Matrix.hpp"
#include "MetalFactory.hpp"

template<typename T>
class MetalGauss {
public:
    MetalFactory *_factory;
    //MTL::Device *_device;
    //MTL::ComputePipelineState *_xorRowPSO;
    //MTL::ComputePipelineState *_partitialPivotingPSO;
    // The command queue used to pass commands to the device.
    MTL::CommandQueue *_mCommandQueue;
    
    // Matrixes
    MTL::Buffer *_mBuffer; // Matrix m
    // MTL::Buffer *_resultBufer; // Result Matrix
    MTL::Buffer *_currentRowBuffer;
    MTL::Buffer *_currentColBuffer;
    MTL::Buffer *_rowsBuffer; // number of all rows of the matrix m
    MTL::Buffer *_bucketsBuffer; // number of all columns of  the matrix m
    MTL::Buffer *_indexResultBuffer; // the pivot index result
    MTL::Buffer *_currentBucketBuffer; // the bucket of the current col
    
    
    MetalGauss(MTL::Device *device, Matrix<T> *m, bool debugPrints = false);
    
    ~MetalGauss();
    void prepareData();
    
    
    
    int sendPartitialPivotComputeCommand(int &currentRow);
    void sendXORComputeCommand(int &currentRow);
    // Solve gauss
    
    Matrix<T> *solve();
    Matrix<T> *solve_noParallelPivoting();
    static void solve_withoutMetal(Matrix<T> *m);
    void gaussForwardReduction();
    
private:
    static void swapRows(Matrix<T> &m, int &l, int &k);
    void printMatrix(Matrix<T> &m);
    void println(std::string s);
    
    int sendPartitialPivotComputeCommand(MTL::ComputePipelineState *pso, int &currentRow);
    void sendXORComputeCommand(MTL::ComputePipelineState *pso, int &currentRow);
    
    Matrix<T> *_m;
    bool _debug;
    
    MTL::ComputePipelineState *pso_xor;
    MTL::ComputePipelineState *pso_pivot;
};

