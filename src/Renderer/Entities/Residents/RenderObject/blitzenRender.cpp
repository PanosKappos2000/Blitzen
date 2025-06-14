#include "blitRender.h"
#include "worldTransform.h"
#include "Renderer/Resources/blitRenderingResources.h"

namespace BlitzenEngine
{
    uint32_t RenderContainer::CreateRenderObject(RENDER_OBJECT_CREATE_CONTEXT& context)
    {
        if (m_renderCount >= BlitzenCore::Ce_MaxRenderObjectCount)
        {
            BLIT_ERROR("Exceeded render object limit");
            return BlitzenCore::Ce_MaxRenderObjectCount;
        } 

        // TODO: check flags
        // Create opaque dynamic
        // Create opaque normal
        // Create

        if (context.m_type == RENDER_OBJECT_TYPE::TRANSPARENT_STATIC)
        {
            BLIT_ERROR("Transparent object are momentarily out of commission");
            return BlitzenCore::Ce_MaxRenderObjectCount;
        }

        auto& newcomer{ m_renders[m_renderCount] };
        newcomer.surfaceId = context.m_primitiveID;
        newcomer.transformId = context.m_transformID;

        return m_renderCount++;
    }

    uint32_t WorldTransformContainer::CreateTransform(const TRANSFORM_CREATE_CONTEXT& context)
    {
        if (m_transformCount >= BlitzenCore::Ce_MaxWorldTransformCount)
        {
            BLIT_ERROR("Exceeded max world transform count");
            return BlitzenCore::Ce_MaxWorldTransformCount;
        }

        // DYNAMIC TRANSFORMS
        if (context.m_type == WorldTransformType::DYNAMIC)
        {
            if (m_dynamicTransformCount >= BlitzenCore::Ce_MaxWorldMovingResidentCount)
            {
                BLIT_ERROR("Exeeded max dynamic transform count");
                return BlitzenCore::Ce_MaxWorldTransformCount;
            }

            auto& newcomer = m_transforms[CE_DYNAMIC_TRANSFORM_OFFSET + m_dynamicTransformCount];

            if (context.m_pTransform != nullptr)
            {
                BlitzenCore::BlitMemCopy<MeshTransform>(&newcomer, context.m_pTransform, 1);
            }
            else
            {
                if (context.m_scale == 0 || context.m_randomTransformMultiplier == 0)
                {
                    BLIT_ERROR("Did not provide for transform creation");
                    return BlitzenCore::Ce_MaxWorldTransformCount;
                }

                RandomizeTransform(newcomer, context.m_randomTransformMultiplier, context.m_scale);
            }
            
            m_transformCount++;
            return CE_DYNAMIC_TRANSFORM_OFFSET + m_dynamicTransformCount++;
        }

        // STATIC TRANSFORMS
        else if (context.m_type == WorldTransformType::STATIC)
        {
            if (m_staticTransformCount >= CE_MAX_STATIC_TRANSFORM_COUNT)
            {
                BLIT_ERROR("Exceeded max static transform count");
                return BlitzenCore::Ce_MaxWorldTransformCount;
            }

            auto& newcomer = m_transforms[CE_STATIC_TRANSFORM_OFFSET + m_staticTransformCount];

            if (context.m_pTransform != nullptr)
            {
                BlitzenCore::BlitMemCopy<MeshTransform>(&newcomer, context.m_pTransform, 1);
            }
            else
            {
                if (context.m_scale == 0 || context.m_randomTransformMultiplier == 0)
                {
                    BLIT_ERROR("Did not provide for transform creation");
                    return BlitzenCore::Ce_MaxWorldTransformCount;
                }

                RandomizeTransform(newcomer, context.m_randomTransformMultiplier, context.m_scale);
            }

            m_transformCount++;
            return CE_STATIC_TRANSFORM_OFFSET + m_staticTransformCount++;
        }

        BLIT_ERROR("Unexpected transform type");
        return BlitzenCore::Ce_MaxWorldTransformCount;
    }

    void RandomizeTransform(MeshTransform& transform, float multiplier, float scale)
    {
        transform.pos = BlitML::vec3((float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier);

        transform.scale = scale;

        transform.orientation = BlitML::QuatFromAngleAxis(BlitML::vec3((float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1), BlitML::Radians((float(rand()) / RAND_MAX) * 90.f), 0);
    }
}