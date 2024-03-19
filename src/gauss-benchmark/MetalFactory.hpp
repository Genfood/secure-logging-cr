//
//  MetalFactory.hpp
//  gauss-benchmark
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 26.10.23.
//

#ifndef MetalFactory_hpp
#define MetalFactory_hpp

#include <stdio.h>
#import <Metal/Metal.hpp>
#include <iostream>

class MetalFactory {
    MTL::Device *_device;
    MTL::CommandQueue *_queue; // The command queue used to pass commands to the device.
    
    
    void encodeComand(MTL::ComputePipelineState *pso, MTL::ComputeCommandEncoder *encoder, const MTL::Buffer * const buffers[], int size, unsigned int threadCount);
    
    void loadFunctions(MTL::Library *lib);
    MTL::ComputePipelineState *getPSO(MTL::Library *lib, std::string funname);
    
public:
    MetalFactory(MTL::Device *device);
    ~MetalFactory();
    MTL::Buffer *newBuffer(size_t size);
    MTL::Buffer *newBuffer(const void *pointer, size_t size);
    void sendCommand(MTL::ComputePipelineState *pso, const MTL::Buffer * const buffers[], int size, unsigned int threadCount);
    
    
    // PSOs
    //MTL::ComputePipelineState *XorPSO;
    //MTL::ComputePipelineState *PivotPSO;
    
    MTL::ComputePipelineState *PSO_xor_1;
    MTL::ComputePipelineState *PSO_xor_32;
    MTL::ComputePipelineState *PSO_xor_64;
    MTL::ComputePipelineState *PSO_xor_256;
    
    
    MTL::ComputePipelineState *PSO_pivot_1;
    MTL::ComputePipelineState *PSO_pivot_32;
    MTL::ComputePipelineState *PSO_pivot_64;
    MTL::ComputePipelineState *PSO_pivot_256;
    
    // classic
    //MTL::ComputePipelineState *XorPSO_classic;
    //MTL::ComputePipelineState *PivotPSO_classic;
    
    // static helper functions
    static void setBufferWithUInt32(MTL::Buffer *buffer, uint32_t data);
    static void setBufferWithInt32(MTL::Buffer *buffer, int data);
    
};

#endif /* MetalFactory_hpp */
