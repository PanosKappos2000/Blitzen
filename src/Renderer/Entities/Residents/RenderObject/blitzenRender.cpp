#include "blitRender.h"
#include "worldTransform.h"
#include "Renderer/Resources/blitRenderingResources.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
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
            BLIT_ERROR("%s: Exceeded max world transform count", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
            return BLIT_MAX_WORLD_TRANSFORM_COUNT;
        }

        // DYNAMIC TRANSFORMS
        if (context.m_type == WorldTransformType::DYNAMIC)
        {
            if (m_moveableCount >= BlitzenCore::Ce_MaxWorldMovingResidentCount)
            {
                BLIT_ERROR("%s: Exeeded max dynamic transform count", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                return CE_TRANSFORM_CREATE_ERROR_CODE;
            }

            if (context.cpu_pTransform == nullptr)
            {
                BLIT_ERROR("%s: Dynamic transform requested but no CPU transform passed", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                return CE_TRANSFORM_CREATE_ERROR_CODE;
            }

            // CPU DATA is needed for transforms to update in the shaders and outside
            BlitzenCore::BlitMemCopy(&m_moveables[m_moveableCount], context.cpu_pTransform, sizeof(CPU_TRANSFORM));

            // Shader side data
            if (context.m_pTransform != nullptr)
            {
                BlitzenCore::BlitMemCopy(&m_transforms[CE_DYNAMIC_TRANSFORM_OFFSET + m_moveableCount], context.m_pTransform, sizeof(MeshTransform));
            }
            else
            {
                BLIT_WARN("%s: Dynamic transform is being created with no initial GPU data", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
            }
            
            m_transformCount++;
            return CE_DYNAMIC_TRANSFORM_OFFSET + m_moveableCount++;
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
                BLIT_ERROR("%s: Did not provide GPU data for static transform creation", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                return BLIT_MAX_WORLD_TRANSFORM_COUNT;
            }

            m_transformCount++;
            return CE_STATIC_TRANSFORM_OFFSET + m_staticTransformCount++;
        }

        BLIT_ERROR("Unexpected transform type");
        return BLIT_MAX_WORLD_TRANSFORM_COUNT;
    }

    void RandomizeTransform(MeshTransform* pTransform, float multiplier, float scale)
    {
        pTransform->pos = BlitML::vec3((float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier);

        pTransform->scale = scale;

        pTransform->orientation = BlitML::QuatFromAngleAxis(BlitML::vec3((float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1), BlitML::Radians((float(rand()) / RAND_MAX) * 90.f), 0);
    }

    void RandomizeTransform(CPU_TRANSFORM* pTransform, float multiplier)
    {
        pTransform->position = BlitML::vec3((float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier);
        pTransform->eulerAngles = BlitML::vec3((float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier);
    }
}