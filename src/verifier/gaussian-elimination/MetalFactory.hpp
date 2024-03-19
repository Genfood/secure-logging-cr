//
//  MetalFactory.hpp
//  verifier
//
//  Copyright © 2023 Airbus Commercial Aircraft
//  Created by Florian on 21.11.23.
//

#ifndef MetalFactory_hpp
#define MetalFactory_hpp

#include <Metal/Metal.hpp>

namespace Gauss {
    class MetalFactory {
        MTL::Device *_device;
        MTL::CommandQueue *_queue; // The command queue used to pass commands to the device.
        
        
        void encodeComand(MTL::ComputePipelineState *pso, MTL::ComputeCommandEncoder *encoder, const MTL::Buffer * const buffers[], int size, unsigned int threadCount, bool useMaxThreadGroupSize);
        
        MTL::ComputePipelineState *getPSO(MTL::Library *lib, std::string funname);
        void loadFunctions(MTL::Library *lib);
        
    public:
        MetalFactory(MTL::Device *device);
        ~MetalFactory();
        MTL::Buffer *newBuffer(size_t size);
        MTL::Buffer *newBuffer(const void *pointer, size_t size);
        void sendCommand(MTL::ComputePipelineState *pso, const MTL::Buffer * const buffers[], int size, unsigned int threadCount, bool useMaxThreadGroupSize);
        
        
        // PSOs
        MTL::ComputePipelineState *XorPSO;
        MTL::ComputePipelineState *PivotPSO;
        MTL::ComputePipelineState *BookkeepingPSO;
        
        // static helper functions
        static void setBufferWithUInt32(MTL::Buffer *buffer, uint32_t data);
        static void setBufferWithInt32(MTL::Buffer *buffer, int data);
        
    };
}

#endif /* MetalFactory_hpp */
