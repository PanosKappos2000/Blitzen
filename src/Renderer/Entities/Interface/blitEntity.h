#pragma once
#include "Core/blitzenEngine.h"
#include "Renderer/Entities/DynamicTransform/blitDynamicTransform.h"
#include "BlitCL/blitPfn.h"

namespace BlitzenEngine
{
    // This might need to point to every other component
    struct Entity
    {
        MeshTransform* m_pTransform{ nullptr };

        uint32_t m_transformId;

        Mesh* m_pMesh{ nullptr };

        DynamicRotation* m_pDynamicRotation{ nullptr };
    };

    enum class ENTITY_UPDATE_RES : uint8_t
    {
        NO_FURTHER_UPDATES = 0, 

        UPDATE_RENDERER_TRANSFORM = 1
    };
    
    using PFN_ENTITY_UPDATE = BlitCL::Pfn<ENTITY_UPDATE_RES, Entity*, float>;

    struct DynamicUpdateEntity
    {
        PFN_ENTITY_UPDATE m_pfnUpdate;

        Entity* pEntity;
    };
}