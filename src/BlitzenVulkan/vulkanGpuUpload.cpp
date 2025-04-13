#include "vulkanRenderer.h"
#include "vulkanResourceFunctions.h"

namespace BlitzenVulkan
{
    VkDeviceSize CreateGlobalSSBOVertexBuffer(VkDevice device, VmaAllocator allocator, uint8_t bRaytracingSupported,
        AllocatedBuffer& stagingVertexBuffer, PushDescriptorBuffer<void>& vertexBuffer, 
        BlitzenEngine::Vertex* pVertexData, size_t vertexDataCount)
    {
        // Raytracing support needs additional flags
        uint32_t geometryBuffersRaytracingFlags = bRaytracingSupported ?
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            : 0;
        VkDeviceSize vertexBufferSize = sizeof(BlitzenEngine::Vertex) * vertexDataCount;
        if (vertexBufferSize == 0)
        {
            return 0;
        }
        // Creates vertex buffer. If the function fails returns 0 to let the caller know
        if (!SetupPushDescriptorBuffer(device, allocator, vertexBuffer, stagingVertexBuffer,
            vertexBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | geometryBuffersRaytracingFlags,
            pVertexData))
        {
            return 0;
        }
        return vertexBufferSize;
    }

    VkDeviceSize CreateGlobalIndexBuffer(VkDevice device, VmaAllocator allocator, uint8_t bRaytracingSupported, 
        AllocatedBuffer& stagingIndexBuffer, AllocatedBuffer& indexBuffer, uint32_t* pIndexData, size_t indicesCount)
    {
        // Raytracing support needs additional flags
        uint32_t geometryBuffersRaytracingFlags = bRaytracingSupported ?
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR 
            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
            : 0;
        VkDeviceSize indexBufferSize = sizeof(uint32_t) * indicesCount;
        if(indexBufferSize == 0)
        {
            return 0;
        }
        // Creates index buffer. If the function fails, return 0 to let the caller know
        CreateStorageBufferWithStagingBuffer(allocator, device, 
            pIndexData, indexBuffer, stagingIndexBuffer, 
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |  geometryBuffersRaytracingFlags, 
            indexBufferSize);
        if(indexBuffer.bufferHandle == VK_NULL_HANDLE)
        {
            return 0;
        }
        return indexBufferSize;
    }
}