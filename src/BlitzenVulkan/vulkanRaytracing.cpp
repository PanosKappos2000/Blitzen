#pragma once
#include "vulkanRenderer.h"
#include "vulkanResourceFunctions.h"

namespace BlitzenVulkan
{
    static VkResult CreateAccelerationStructureKHR(VkInstance instance, VkDevice device,
        const VkAccelerationStructureCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator,
        VkAccelerationStructureKHR* pAccelerationStructure)
    {
        auto func = (PFN_vkCreateAccelerationStructureKHR)vkGetInstanceProcAddr(
            instance, "vkCreateAccelerationStructureKHR");
        if (func != nullptr)
        {
            return func(device, pCreateInfo, pAllocator, pAccelerationStructure);
        }

        return VK_NOT_READY;
    }

    static void BuildAccelerationStructureKHR(VkInstance instance, VkCommandBuffer commandBuffer, uint32_t infoCount,
        const VkAccelerationStructureBuildGeometryInfoKHR* buildInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppRangeInfos)
    {
        auto func = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetInstanceProcAddr(
            instance, "vkCmdBuildAccelerationStructuresKHR");
        if (func != nullptr)
        {
            func(commandBuffer, infoCount, buildInfos, ppRangeInfos);
        }
    }

    static void GetAccelerationStructureBuildSizesKHR(VkInstance instance, VkDevice device,
        VkAccelerationStructureBuildTypeKHR buildType, const VkAccelerationStructureBuildGeometryInfoKHR* pBuildInfo,
        const uint32_t* pMaxPrimitiveCounts, VkAccelerationStructureBuildSizesInfoKHR* pBuildSizes)
    {
        auto func = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetInstanceProcAddr(
            instance, "vkGetAccelerationStructureBuildSizesKHR");
        if (func != nullptr)
        {
            func(device, buildType, pBuildInfo, pMaxPrimitiveCounts, pBuildSizes);
        }
    }

    static VkDeviceAddress GetAccelerationStructureDeviceAddressKHR(VkInstance instance, VkDevice device,
        const VkAccelerationStructureDeviceAddressInfoKHR* pInfo)
    {
        auto func = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetInstanceProcAddr(
            instance, "vkGetAccelerationStructureDeviceAddressKHR");
        if (func != nullptr)
        {
            return func(device, pInfo);
        }

        return (VkDeviceAddress)VK_NULL_HANDLE;
    }

    uint8_t VulkanRenderer::BuildBlas(BlitzenEngine::RenderingResources* pResources)
    {
        auto& surfaces = pResources->GetSurfaceArray();
		auto& primitiveVertexCounts = pResources->GetPrimitiveVertexCounts();
		auto& lods = pResources->GetLodData();
        if (!surfaces.GetSize())
        {
            return 0;
        }

        BlitCL::DynamicArray<uint32_t> primitiveCounts(surfaces.GetSize());
        BlitCL::DynamicArray<VkAccelerationStructureGeometryKHR> geometries(surfaces.GetSize(), {});
        BlitCL::DynamicArray<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos(surfaces.GetSize(), {});

        BlitCL::DynamicArray<size_t> accelerationOffsets(surfaces.GetSize());
        BlitCL::DynamicArray<size_t> accelerationSizes(surfaces.GetSize());
        BlitCL::DynamicArray<size_t> scratchOffsets(surfaces.GetSize());

        const size_t ce_alignment = 256; // required by spec for acceleration structures

        size_t totalAccelerationSize = 0;
        size_t totalScratchSize = 0;

        // Gets the address of the vertex buffer and then the index buffer
        VkDeviceAddress vertexBufferAddress = GetBufferAddress(m_device,
            m_currentStaticBuffers.vertexBuffer.buffer.bufferHandle);
        VkDeviceAddress indexBufferAddress = GetBufferAddress(m_device,
            m_currentStaticBuffers.indexBuffer.bufferHandle);

        for (size_t i = 0; i < surfaces.GetSize(); ++i)
        {
            const auto& surface = surfaces[i];
            VkAccelerationStructureGeometryKHR& geometry = geometries[i];// The as geometry that will be initialized in this loop
            VkAccelerationStructureBuildGeometryInfoKHR& buildInfo = buildInfos[i];

            // Specifies the geometry for this accelration structure
            geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
            geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            geometry.pNext = nullptr;
            geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;

            geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            geometry.geometry.triangles.pNext = nullptr;

            // Passing vertex data
            geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            // Gets the precise address of the vertex buffer for the current surface (needs to be incremented by the vertex offset)
            geometry.geometry.triangles.vertexData.deviceAddress =
                static_cast<VkDeviceAddress>(vertexBufferAddress + surface.vertexOffset * sizeof(BlitzenEngine::Vertex));
            geometry.geometry.triangles.vertexStride = sizeof(BlitzenEngine::Vertex);
            // Primitive vertex count (at the moment), is an array created for this one, optinal line of code
            geometry.geometry.triangles.maxVertex = primitiveVertexCounts[i];

            // Passing index data
            geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
            // Precise address of the index buffer
            geometry.geometry.triangles.indexData.deviceAddress =
                static_cast<VkDeviceAddress>(indexBufferAddress + lods[surface.lodOffset].firstIndex * sizeof(uint32_t));


            // Build info for the acceleration structu. Takes the geometry struct from above and some other configs
            buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
            buildInfo.pNext = nullptr;
            buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;// Bottom level since the as is for geometries
            buildInfo.flags =
                VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
                VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_COMPACTION_BIT_KHR;
            buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            buildInfo.geometryCount = 1;
            buildInfo.pGeometries = &geometry;


            primitiveCounts[i] = lods[surface.lodOffset].indexCount / 3;

            VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
            sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
            GetAccelerationStructureBuildSizesKHR(m_instance, m_device,
                VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &primitiveCounts[i], &sizeInfo);

            accelerationOffsets[i] = totalAccelerationSize;
            accelerationSizes[i] = sizeInfo.accelerationStructureSize;
            scratchOffsets[i] = totalScratchSize;

            totalAccelerationSize = (totalAccelerationSize + sizeInfo.accelerationStructureSize +
                ce_alignment - 1) & ~(ce_alignment - 1);
            totalScratchSize = (totalScratchSize + sizeInfo.buildScratchSize + ce_alignment - 1) & ~(ce_alignment - 1);
        }

        if (!CreateBuffer(m_allocator, m_currentStaticBuffers.blasBuffer,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY, totalAccelerationSize, 0))
            return 0;

        AllocatedBuffer stagingBuffer;
        if (!CreateBuffer(m_allocator, stagingBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY, totalScratchSize, 0))
            return 0;

        VkDeviceAddress blasStagingBufferAddress = GetBufferAddress(m_device, stagingBuffer.bufferHandle);

        m_currentStaticBuffers.blasData.Resize(surfaces.GetSize());

        // Need to empty-brace initialize every damned struct otherwise Vulkan breaks
        BlitCL::DynamicArray<VkAccelerationStructureBuildRangeInfoKHR> buildRanges(surfaces.GetSize(), {});
        BlitCL::DynamicArray<const VkAccelerationStructureBuildRangeInfoKHR*> buildRangePtrs(surfaces.GetSize());

        for (size_t i = 0; i < surfaces.GetSize(); ++i)
        {
            VkAccelerationStructureCreateInfoKHR accelerationInfo{};
            accelerationInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
            accelerationInfo.buffer = m_currentStaticBuffers.blasBuffer.bufferHandle;
            accelerationInfo.offset = accelerationOffsets[i];
            accelerationInfo.size = accelerationSizes[i];
            accelerationInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

            if (CreateAccelerationStructureKHR(m_instance, m_device, &accelerationInfo, nullptr,
                &m_currentStaticBuffers.blasData[i].handle) != VK_SUCCESS)
                return 0;

            buildInfos[i].dstAccelerationStructure = m_currentStaticBuffers.blasData[i].handle;
            buildInfos[i].scratchData.deviceAddress = blasStagingBufferAddress + scratchOffsets[i];

            VkAccelerationStructureBuildRangeInfoKHR& buildRange = buildRanges[i];
            buildRanges[i].primitiveCount = primitiveCounts[i];
            buildRangePtrs[i] = &buildRanges[i];
        }

        VkCommandBuffer commandBuffer = m_frameToolsList[0].transferCommandBuffer;
        BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        BuildAccelerationStructureKHR(m_instance, commandBuffer, static_cast<uint32_t>(surfaces.GetSize()),
            buildInfos.Data(), buildRangePtrs.Data());
        SubmitCommandBuffer(m_transferQueue.handle, commandBuffer);
        vkQueueWaitIdle(m_transferQueue.handle);

        return 1;
    }

    uint8_t VulkanRenderer::BuildTlas(BlitzenEngine::RenderObject* pDraws, uint32_t drawCount,
        BlitzenEngine::MeshTransform* pTransforms, BlitzenEngine::PrimitiveSurface* pSurfaces)
    {
        // Retrieves the device address of each acceleration structure that was build earlier
        BlitCL::DynamicArray<VkDeviceAddress> blasAddresses{ m_currentStaticBuffers.blasData.GetSize() };
        for (size_t i = 0; i < blasAddresses.GetSize(); ++i)
        {
            VkAccelerationStructureDeviceAddressInfoKHR	addressInfo{};
            addressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
            addressInfo.accelerationStructure = m_currentStaticBuffers.blasData[i].handle;
            blasAddresses[i] = GetAccelerationStructureDeviceAddressKHR(m_instance, m_device, &addressInfo);
        }

        // Creates an object buffer that will hold a VkAccelerationsStructureInstanceKHR for each object loaded
        AllocatedBuffer objectBuffer;
        if (!CreateBuffer(m_allocator, objectBuffer,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR
            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU,
            sizeof(VkAccelerationStructureInstanceKHR) * drawCount, VMA_ALLOCATION_CREATE_MAPPED_BIT
        ))
            return 0;

        // Creates a VkAccelrationStructureInstanceKHR for each instance's transform and copies it to the object buffer
        for (size_t i = 0; i < drawCount; ++i)
        {
            const auto& object = pDraws[i];
            const auto& transform = pTransforms[object.transformId];
            const auto& surface = pSurfaces[object.surfaceId];

            // Casts the orientation quat to a matrix
            auto orientationTranspose{ BlitML::Transpose(BlitML::QuatToMat4(transform.orientation)) };
            auto xform = orientationTranspose * transform.scale;

            VkAccelerationStructureInstanceKHR instance{};
            // Copies the first 3 elements of the 1st row of the matrix to the 1st row of the Vulkan side matrix
            BlitzenCore::BlitMemCopy(instance.transform.matrix[0], &xform[0], sizeof(float) * 3);
            // Copies the first 3 elements of the 2nd row of the matrix to the 2nd row of the Vulkan side matrix
            BlitzenCore::BlitMemCopy(instance.transform.matrix[1], &xform[4], sizeof(float) * 3);
            // Copies the first 3 elements of the 3rd row of the matrix to the 3rd row of the Vulkan side matrix
            BlitzenCore::BlitMemCopy(instance.transform.matrix[2], &xform[8], sizeof(float) * 3);

            instance.transform.matrix[0][3] = transform.pos.x;
            instance.transform.matrix[1][3] = transform.pos.y;
            instance.transform.matrix[2][3] = transform.pos.z;

            instance.instanceCustomIndex = i;
            instance.mask = 1 << 0/*surface.postPass*/; // No transparent objects are passed here, 
            // TODO: Fix the above
            instance.flags = /*surface.postPass*/ false ? VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR : 0;
            instance.accelerationStructureReference = blasAddresses[object.surfaceId];

            auto pData = reinterpret_cast<VkAccelerationStructureInstanceKHR*>(
                objectBuffer.allocationInfo.pMappedData) + i;
            BlitzenCore::BlitMemCopy(pData, &instance, sizeof(VkAccelerationStructureInstanceKHR));
        }

        VkAccelerationStructureGeometryKHR geometry{};
        geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
        geometry.geometry.instances.data.deviceAddress = GetBufferAddress(m_device, objectBuffer.bufferHandle);

        // Initial values for build info, more data will become available later in the function
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{};
        buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;// Top level since the as is for instances
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; // Could have different modes for this
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;

        // Gets the size
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{};
        sizeInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
        // Gets the acceleration structure build sizes
        GetAccelerationStructureBuildSizesKHR(m_instance, m_device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
            &buildInfo, &drawCount, &sizeInfo);

        // Creates the Tlas buffer based on the build size that was retrieved above
        // For raytracing, the below struct needs to be added to the pNext chain of the descriptor set layout
        if (!SetupPushDescriptorBuffer<uint8_t>(m_allocator, VMA_MEMORY_USAGE_GPU_ONLY,
            m_currentStaticBuffers.tlasBuffer, sizeInfo.accelerationStructureSize,
            VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR)
            )
        {
            BLIT_ERROR("Failed to create TLAS buffer");
            return 0;
        }

        AllocatedBuffer stagingBuffer;
        if (!CreateBuffer(m_allocator, stagingBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY, sizeInfo.buildScratchSize, 0))
            return 0;

        // Creates the accleration structure
        VkAccelerationStructureCreateInfoKHR accelerationInfo{};
        accelerationInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
        accelerationInfo.buffer = m_currentStaticBuffers.tlasBuffer.buffer.bufferHandle;
        accelerationInfo.size = sizeInfo.accelerationStructureSize;
        accelerationInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        if (CreateAccelerationStructureKHR(m_instance, m_device, &accelerationInfo,
            nullptr, &m_currentStaticBuffers.tlasData.handle) != VK_SUCCESS)
            return 0;

        buildInfo.dstAccelerationStructure = m_currentStaticBuffers.tlasData.handle;
        buildInfo.scratchData.deviceAddress = GetBufferAddress(m_device, stagingBuffer.bufferHandle);

        VkAccelerationStructureBuildRangeInfoKHR buildRange = {};
        buildRange.primitiveCount = drawCount;
        const VkAccelerationStructureBuildRangeInfoKHR* pBuildRange = &buildRange;

        VkCommandBuffer commandBuffer = m_frameToolsList[0].transferCommandBuffer;
        BeginCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        BuildAccelerationStructureKHR(m_instance, commandBuffer, 1, &buildInfo, &pBuildRange);
        SubmitCommandBuffer(m_transferQueue.handle, commandBuffer);
        vkQueueWaitIdle(m_transferQueue.handle);

        return 1;
    }
}