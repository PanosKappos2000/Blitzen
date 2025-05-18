#pragma once
#include <cstdint>

namespace BlitzenCore
{
    enum class AllocationType : uint8_t
    {  
        DynamicArray = 0,
        Hashmap = 1,
        Queue = 2, 
        Bst = 3,
        String = 4, 
        Engine = 5, 
        Renderer = 6, 
        Entity = 7,
        EntityNode = 8,
        Scene = 9,
        SmartPointer = 10,
        LinearAlloc = 11,

        MaxTypes = 12
    };

    constexpr uint16_t Ce_KeyCallbackCount = 256;

    constexpr uint32_t Ce_WorldContextSystemsCount = 5;

    constexpr uint32_t Ce_WorldContextEngineId = 0;
    constexpr uint32_t Ce_WorldContextCameraId = 1;
    constexpr uint32_t Ce_WorldContextWorldTimerManagerId = 2;
    constexpr uint32_t Ce_WorldContextRendererId = 3;
    constexpr uint32_t Ce_WorlContextRndResourcesId = 4;
}

namespace BlitCL
{
    // Hashmap
    constexpr BlitzenCore::AllocationType MapAlloc = BlitzenCore::AllocationType::Hashmap;

    // SmartPointer
    constexpr BlitzenCore::AllocationType SpnAlloc = BlitzenCore::AllocationType::SmartPointer;

    // String
    constexpr uint8_t ce_blitStringCapacityMultiplier = 2;
    constexpr BlitzenCore::AllocationType StrAlloc = BlitzenCore::AllocationType::String;

    // Dynamic array
    constexpr uint8_t ce_blitDynamiArrayCapacityMultiplier = 2;
    constexpr BlitzenCore::AllocationType DArrayAlloc = BlitzenCore::AllocationType::DynamicArray;
}