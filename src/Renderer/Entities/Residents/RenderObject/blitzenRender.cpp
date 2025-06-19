#include "blitRender.h"
#include "worldTransform.h"
#include "Renderer/Resources/blitRenderingResources.h"
#include "Core/DbLog/blitLogger.h"
#include "BlitzenMathLibrary/blitML.h"

namespace BlitzenEngine
{
    uint32_t RenderContainer::CreateRenderObject(RENDER_OBJECT_CREATE_CONTEXT& context)
    {
        if (RENDER_COUNT >= BLIT_MAX_WORLD_RENDERS)
        {
            BLIT_ERROR("%s: Exceeded max render object limit", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
            return BLIT_MAX_WORLD_RENDERS;
        } 

        switch (context.m_type)
        {
        case OPAQUE_STATIC:
        {
            if (m_opaqueStaticCount > BLIT_MAX_WORLD_OPAQUE_STATIC_RENDERS)
            {
                BLIT_ERROR("%s: Exceeded max opaque render object limit", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                return BLIT_MAX_WORLD_RENDERS;
            }

            auto& newcomer{ m_renders[m_opaqueStaticCount + BLIT_OPAQUE_STATIC_RENDER_OFFSET] };

            newcomer.surfaceId = context.m_primitiveID;
            newcomer.transformId = context.m_transformID;

            RENDER_COUNT++;
            return BLIT_OPAQUE_STATIC_RENDER_OFFSET + m_opaqueStaticCount++;
        }

        case OPAQUE_DYNAMIC:
        {
            if (m_opaqueDynamicCount > BLIT_MAX_WORLD_OPAQUE_DYNAMIC_RENDERS)
            {
                BLIT_ERROR("%s: Max opaque dynamic render object reached", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                return BLIT_MAX_WORLD_RENDERS;
            }

            auto& newcomer{ m_renders[m_opaqueDynamicCount + BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET] };

            newcomer.surfaceId = context.m_primitiveID;
            newcomer.transformId = context.m_transformID;

            RENDER_COUNT++;
            return BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET + m_opaqueDynamicCount++;
        }

        case TRANSPARENT_DYNAMIC:
        {
            BLIT_ERROR("%s: No support for dynamic transparent renders for now", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
            return BLIT_MAX_WORLD_RENDERS;
        }

        case TRANSPARENT_STATIC:
        {
            if (m_transparentStaticCount > BLIT_MAX_WORLD_TRANSPARENT_RENDERS)
            {
                BLIT_ERROR("%s: Exceeded max transparent render object limit", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                return BLIT_MAX_WORLD_RENDERS;
            }

            auto& newcomer{ m_renders[m_transparentStaticCount + BLIT_TRANSPARENT_RENDER_OFFSET] };

            newcomer.surfaceId = context.m_primitiveID;
            newcomer.transformId = context.m_transformID;

            RENDER_COUNT++;
            return BLIT_TRANSPARENT_RENDER_OFFSET + m_transparentStaticCount++;
        }
        }

        BLIT_ERROR("%s: Unexpected render object creation path", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
        return BLIT_MAX_WORLD_RENDERS;
    }

    uint32_t WorldTransformContainer::CreateTransform(const TRANSFORM_CREATE_CONTEXT& context)
    {
        if (m_transformCount >= BLIT_MAX_WORLD_TRANSFORM_COUNT)
        {
            BLIT_ERROR("Exceeded max world transform count");
            return BLIT_MAX_WORLD_TRANSFORM_COUNT;
        }

        // DYNAMIC TRANSFORMS
        if (context.m_type == WorldTransformType::DYNAMIC)
        {
            if (m_dynamicTransformCount >= BlitzenCore::Ce_MaxWorldMovingResidentCount)
            {
                BLIT_ERROR("Exeeded max dynamic transform count");
                return BLIT_MAX_WORLD_TRANSFORM_COUNT;
            }

            if (context.m_pTransform != nullptr)
            {
                BlitzenCore::BlitMemCopy(&m_transforms[CE_DYNAMIC_TRANSFORM_OFFSET + m_staticTransformCount], context.m_pTransform, sizeof(MeshTransform));
            }
            else
            {
                if (context.m_scale == 0 || context.m_randomTransformMultiplier == 0)
                {
                    BLIT_ERROR("Did not provide for transform creation");
                    return BLIT_MAX_WORLD_TRANSFORM_COUNT;
                }

                RandomizeTransform(m_transforms[CE_DYNAMIC_TRANSFORM_OFFSET + m_staticTransformCount], context.m_randomTransformMultiplier, context.m_scale);
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
                return BLIT_MAX_WORLD_TRANSFORM_COUNT;
            }

            if (context.m_pTransform != nullptr)
            {
                BlitzenCore::BlitMemCopy(&m_transforms[CE_STATIC_TRANSFORM_OFFSET + m_staticTransformCount], context.m_pTransform, sizeof(MeshTransform));
            }
            else
            {
                if (context.m_scale == 0 || context.m_randomTransformMultiplier == 0)
                {
                    BLIT_ERROR("Did not provide for transform creation");
                    return BLIT_MAX_WORLD_TRANSFORM_COUNT;
                }

                RandomizeTransform(m_transforms[CE_STATIC_TRANSFORM_OFFSET + m_staticTransformCount], context.m_randomTransformMultiplier, context.m_scale);
            }

            m_transformCount++;
            return CE_STATIC_TRANSFORM_OFFSET + m_staticTransformCount++;
        }

        BLIT_ERROR("Unexpected transform type");
        return BLIT_MAX_WORLD_TRANSFORM_COUNT;
    }

    void RandomizeTransform(MeshTransform& transform, float multiplier, float scale)
    {
        transform.pos = BlitML::vec3((float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier);

        transform.scale = scale;

        transform.orientation = BlitML::QuatFromAngleAxis(BlitML::vec3((float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1), BlitML::Radians((float(rand()) / RAND_MAX) * 90.f), 0);
    }
}