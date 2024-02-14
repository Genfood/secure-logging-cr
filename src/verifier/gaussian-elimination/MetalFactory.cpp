//
//  MetalFactory.cpp
//  verifier
//
//  Created by Florian on 21.11.23.
//

#include "MetalFactory.hpp"
#include <stdio.h>
#include <iostream>
#include <math.h>

namespace Gauss{
    MetalFactory::MetalFactory(MTL::Device *device) {
        _device = device;
        
        // Load the shader files with a .metal file extension in the project
        MTL::Library *defaultLibrary = _device->newDefaultLibrary();
        if (defaultLibrary == nullptr) {
            std::cerr << "ERROR: Failed to find the default library." << std::endl;
            exit(EXIT_FAILURE);
        }
        
        MetalFactory::loadFunctions(defaultLibrary);
        defaultLibrary->release();
        
        // Typically, you create one or more command queues when your app launches and then keep them throughout your app’s lifetime.
        _queue = _device->newCommandQueue();
        if (_queue == nullptr) {
            std::cerr << "ERROR: Failed to find the command queue." << std::endl;
            exit(EXIT_FAILURE);
        }
        
    }
    
    void MetalFactory::sendCommand(MTL::ComputePipelineState *pso, const MTL::Buffer * const buffers[], int size, unsigned int threadCount, bool useMaxThreadGroupSize) {
        MTL::CommandBuffer *commandBuffer = _queue->commandBuffer();
        if (commandBuffer == nullptr) {
            std::cerr << "ERROR: Failed to create command buffer." << std::endl;
            exit(EXIT_FAILURE);
        }
        
        MTL::ComputeCommandEncoder *encoder = commandBuffer->computeCommandEncoder();
        if (encoder == nullptr) {
            std::cerr << "ERROR: Failed to create command encoder." << std::endl;
            exit(EXIT_FAILURE);
        }
        
        encodeComand(pso, encoder, buffers, size, threadCount, useMaxThreadGroupSize);
        
        // end the compute pass.
        encoder->endEncoding();
        
        // execute command
        commandBuffer->commit();
        
        // Normally, you want to do other work in your app while the GPU is running,
        // but in this example, the code simply blocks until the calculation is complete.
        commandBuffer->waitUntilCompleted();
        commandBuffer->release();
    }
    
    void MetalFactory::loadFunctions(MTL::Library *lib) {
        XorPSO = getPSO(lib, "xor_row");
        PivotPSO = getPSO(lib, "partial_pivoting");
        BookkeepingPSO = getPSO(lib, "bookkeeping");
    }
    
    MTL::ComputePipelineState *MetalFactory::getPSO(MTL::Library *lib, std::string funName) {
        NS::Error *error = nullptr;
        auto str = NS::String::string(funName.data(), NS::ASCIIStringEncoding);
        
        // Create Functions
        MTL::Function *fun = lib->newFunction(str);
        if (fun == nullptr) {
            std::cerr << "Failed to find the " << fun << " function." << std::endl;
            exit(EXIT_FAILURE);
        }
        
        // create PSOs
        auto pso = _device->newComputePipelineState(fun, &error);
        fun->release();
        
        if (pso == nullptr) {
            std::cerr << "Failed to created pipeline state object." << std::endl;
        }
        
        return pso;
    }
    
    void MetalFactory::encodeComand(MTL::ComputePipelineState *pso, MTL::ComputeCommandEncoder *encoder, const MTL::Buffer * const buffers[], int size, unsigned int threadCount, bool useMaxThreadGroupSize) {
        encoder->setComputePipelineState(pso);
        for (unsigned int i = 0; i < size; ++i) {
            encoder->setBuffer(buffers[i], 0, i);
        }
        
        //NS::UInteger threadCountValue = threadCount;
        encoder->setBytes(&threadCount, sizeof(threadCount), size); // Index size
        
        // Calculate the grid size, the toatl amount of threads needed.
        MTL::Size gridSize = MTL::Size(threadCount, 1, 1);
        
        // optimize parallelization, by setting a good threadgroup size.
        NS::UInteger threadGroupSize;
        NS::UInteger max = pso->maxTotalThreadsPerThreadgroup();
        // Calculate a threadgroup size.
        if (!useMaxThreadGroupSize) {
            NS::UInteger width = pso->threadExecutionWidth(); // Threads per thread group
            threadGroupSize = MIN(width, max);
        } else {
            threadGroupSize = max;
        }
        // Calculate a threadgroup size.
        if (threadGroupSize > threadCount)
        {
            threadGroupSize = threadCount;
        }
        
        MTL::Size threadgroupSize = MTL::Size(threadGroupSize, 1, 1);
        
        // Encode the compute command.
        encoder->dispatchThreads(gridSize, threadgroupSize);
    }
    
    MTL::Buffer *MetalFactory::newBuffer(size_t size)
    {
        return _device->newBuffer(size, MTL::ResourceStorageModeShared);
    }
    
    MTL::Buffer *MetalFactory::newBuffer(const void *pointer, size_t size)
    {
        return _device->newBuffer(pointer, size, MTL::ResourceStorageModeShared);
    }
    
    void MetalFactory::setBufferWithUInt32(MTL::Buffer *buffer, uint32_t data)
    {
        uint32_t *bufferPtr = (uint32_t*)buffer->contents();
        *bufferPtr = data;
    }
    
    void MetalFactory::setBufferWithInt32(MTL::Buffer *buffer, int32_t data)
    {
        int32_t *bufferPtr = (int32_t*)buffer->contents();
        *bufferPtr = data;
    }
    
    
    
    MetalFactory::~MetalFactory() {
        // clean:
        // others
        _queue->release();
        
        // PSOs:
        XorPSO->release();
        PivotPSO->release();
        BookkeepingPSO->release();
    }
}
