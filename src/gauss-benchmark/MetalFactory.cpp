//
//  MetalFactory.cpp
//  gauss-benchmark
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 26.10.23.
//

#include "MetalFactory.hpp"


MetalFactory::MetalFactory(MTL::Device *device) {
    _device = device;
    
    // Load the shader files with a .metal file extension in the project
    MTL::Library *defaultLibrary = _device->newDefaultLibrary();
    assert(defaultLibrary != nullptr && "Failed to find the default library.");
    
    MetalFactory::loadFunctions(defaultLibrary);
    defaultLibrary->release();

    // Typically, you create one or more command queues when your app launches and then keep them throughout your app’s lifetime.
    _queue = _device->newCommandQueue();
    assert(_queue != nullptr && "Failed to find the command queue..");
    
}

void MetalFactory::sendCommand(MTL::ComputePipelineState *pso, const MTL::Buffer * const buffers[], int size, unsigned int threadCount) {
    MTL::CommandBuffer *commandBuffer = _queue->commandBuffer();
    assert(commandBuffer != nullptr);
    
    MTL::ComputeCommandEncoder *encoder = commandBuffer->computeCommandEncoder();
    assert(encoder != nullptr);
    
    encodeComand(pso, encoder, buffers, size, threadCount);
    
    // end the compute pass.
    encoder->endEncoding();
    
    // execute command
    commandBuffer->commit();

    // Normally, you want to do other work in your app while the GPU is running,
    // but in this example, the code simply blocks until the calculation is complete.
    commandBuffer->waitUntilCompleted();
    commandBuffer->release();
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

void MetalFactory::loadFunctions(MTL::Library *lib) {
    PSO_xor_1 = getPSO(lib, "xor_row_1");
    PSO_xor_32 = getPSO(lib, "xor_row_32");
    PSO_xor_64 = getPSO(lib, "xor_row_64");
    PSO_xor_256 = getPSO(lib, "xor_row_256");
    
    PSO_pivot_1 = getPSO(lib, "partial_pivoting_1");
    PSO_pivot_32 = getPSO(lib, "partial_pivoting_32");
    PSO_pivot_64 = getPSO(lib, "partial_pivoting_64");
    PSO_pivot_256 = getPSO(lib, "partial_pivoting_256");
}

void MetalFactory::encodeComand(MTL::ComputePipelineState *pso, MTL::ComputeCommandEncoder *encoder, const MTL::Buffer * const buffers[], int size, unsigned int threadCount) {
        
    encoder->setComputePipelineState(pso);
    for (unsigned int i = 0; i < size; ++i) {
        encoder->setBuffer(buffers[i], 0, i);
    }
    // Set all buffers at once
    //encoder->setBuffers(buffers, 0, NS::Range(0, size));
    
    NS::UInteger threadCountValue = threadCount;
    encoder->setBytes(&threadCountValue, sizeof(threadCountValue), size); // Index size
    
    // Calculate the grid size, the toatl amount of threads needed.
    MTL::Size gridSize = MTL::Size(threadCount, 1, 1);
    
    // optimize parallelization, by setting a good threadgroup size.
    NS::UInteger w = pso->threadExecutionWidth(); // Threads per thread group
    // Calculate a threadgroup size.
    NS::UInteger threadGroupSize = w; //pso->maxTotalThreadsPerThreadgroup();
    //NS::UInteger threadGroupSize = pso->maxTotalThreadsPerThreadgroup();
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
    PSO_xor_1->release();
    PSO_xor_32->release();
    PSO_xor_64->release();
    PSO_xor_256->release();
    
    PSO_pivot_1->release();
    PSO_pivot_32->release();
    PSO_pivot_64->release();
    PSO_pivot_256->release();
}

