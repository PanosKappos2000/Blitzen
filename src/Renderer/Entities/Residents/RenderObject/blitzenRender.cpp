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
            return GCRenderObjectCreationErrorCode;
        } 

        switch (context.m_type)
        {
        case OPAQUE_STATIC:
        {
            if (m_opaqueStaticCount > BLIT_MAX_WORLD_OPAQUE_STATIC_RENDERS)
            {
                BLIT_ERROR("%s: Exceeded max opaque render object limit", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                return GCRenderObjectCreationErrorCode;
            }

            auto& newcomer{ m_renders[m_opaqueStaticCount + BLIT_OPAQUE_STATIC_RENDER_OFFSET] };
            newcomer.surfaceId = context.m_primitiveID;
            // OBSOLETE
            newcomer.transformId = context.m_transformID;

            // Success, increments general and specific count
            RENDER_COUNT++;
            return BLIT_OPAQUE_STATIC_RENDER_OFFSET + m_opaqueStaticCount++;
        }

        case OPAQUE_DYNAMIC:
        {
            if (m_opaqueDynamicCount > BLIT_MAX_WORLD_OPAQUE_DYNAMIC_RENDERS)
            {
                BLIT_ERROR("%s: Max opaque dynamic render object reached", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                return GCRenderObjectCreationErrorCode;
            }

            auto& newcomer{ m_renders[m_opaqueDynamicCount + BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET] };

            newcomer.surfaceId = context.m_primitiveID;
            newcomer.transformId = context.m_transformID;

            // Success, increments general and specific count
            RENDER_COUNT++;
            return BLIT_OPAQUE_DYNAMIC_RENDER_OFFSET + m_opaqueDynamicCount++;
        }

        case TRANSPARENT_DYNAMIC:
        {
            BLIT_ERROR("%s: No support for dynamic transparent renders for now", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
            return GCRenderObjectCreationErrorCode;
        }

        case TRANSPARENT_STATIC:
        {
            if (m_transparentStaticCount > BLIT_MAX_WORLD_TRANSPARENTS)
            {
                BLIT_ERROR("%s: Exceeded max transparent render object limit", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                return GCRenderObjectCreationErrorCode;
            }

            auto& newcomer{ m_renders[m_transparentStaticCount + BLIT_TRANSPARENT_RENDER_OFFSET] };

            newcomer.surfaceId = context.m_primitiveID;
            newcomer.transformId = context.m_transformID;

            RENDER_COUNT++;
            return BLIT_TRANSPARENT_RENDER_OFFSET + m_transparentStaticCount++;
        }
        }

        BLIT_ERROR("%s: Unexpected render object creation path", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
        return GCRenderObjectCreationErrorCode;
    }

    uint32_t WorldTransformContainer::CreateTransform(const TRANSFORM_CREATE_CONTEXT& context)
    {
        if (m_transformCount >= BLIT_MAX_WORLD_TRANSFORM_COUNT)
        {
            BLIT_ERROR("%s: Exceeded max world transform count", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
            return GCTransformCreateErrorCode;
        }

        if (context.m_pTransform == nullptr)
        {
            BLIT_ERROR("%s: No GPU data given for transform", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
            return GCTransformCreateErrorCode;
        }

        // Check that the scale of the objects does not surpass the size of a cell in the world partition grid
        // NOTE: I am not confident that this check is good.
        if (context.m_pTransform->scale <= 0.f || context.m_pTransform->scale > BLIT_COLLISION_GRID_CELL_EXTENT)
        {
			BLIT_ERROR("%s: Invalid scale passed to transform creation: %f", BlitzenCore::CE_RESIDENT_SYSTEM_NAME, context.m_pTransform->scale);
			return GCTransformCreateErrorCode;
        }

        // Transforms for world variables (possible dynamic logic)
        if (context.m_type == WorldTransformType::DYNAMIC)
        {
            //-------------------------------------------------------------------------------------------------------------------------------
            // CPU data. Helps communicate with the GPU about game logic, without loading the CPU with heavy computations like quats
            //-------------------------------------------------------------------------------------------------------------------------------
            if (m_moveableCount >= BLIT_MAX_WORLD_VARIABLE_COUNT)
            {
                BLIT_ERROR("%s: Exeeded max dynamic transform count", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                return GCTransformCreateErrorCode;
            }
            if (context.cpu_pTransform == nullptr)
            {
                BLIT_ERROR("%s: Dynamic transform requested but no CPU transform data passed", BlitzenCore::CE_RESIDENT_SYSTEM_NAME);
                return GCTransformCreateErrorCode;
            }
            BlitzenCore::BlitMemCopy(&WVWithMovement[m_moveableCount], context.cpu_pTransform, sizeof(WVTransform));

            // GPU data, checked for null before the if block.
            BlitzenCore::BlitMemCopy(&m_transforms[CE_DYNAMIC_TRANSFORM_OFFSET + m_moveableCount], context.m_pTransform, sizeof(MeshTransform));
            // Sync position.
			context.cpu_pTransform->position = context.m_pTransform->pos;
            // Sync orientation (quat gen)
            auto& transform{ m_transforms[CE_DYNAMIC_TRANSFORM_OFFSET + m_moveableCount] };
            BlitML::quat orientationYaw = BlitML::NormalizedQuatFromAngleAxis(BlitML::float3(0.f, -1.f, 0.f), context.cpu_pTransform->eulerAngles.x);
            BlitML::quat orientationPitch = BlitML::NormalizedQuatFromAngleAxis(BlitML::float3(1.f, 0.f, 0.f), context.cpu_pTransform->eulerAngles.y);
            transform.orientation = BlitML::MulitplyQuat(orientationYaw, orientationPitch);

            // Success, increment general and dynamic
            m_transformCount++;
            return CE_DYNAMIC_TRANSFORM_OFFSET + m_moveableCount++;
        }

        // STATIC TRANSFORMS
        else if (context.m_type == WorldTransformType::STATIC)
        {
            if (m_staticTransformCount >= CE_MAX_STATIC_TRANSFORM_COUNT)
            {
                BLIT_ERROR("Exceeded max static transform count");
                return GCTransformCreateErrorCode;
            }

            BlitzenCore::BlitMemCopy(&m_transforms[CE_STATIC_TRANSFORM_OFFSET + m_staticTransformCount], context.m_pTransform, sizeof(MeshTransform));

            m_transformCount++;
            return CE_STATIC_TRANSFORM_OFFSET + m_staticTransformCount++;
        }

        // TRANSPARENT OBJECTS (not supported for now)
        else if (context.m_type == WorldTransformType::BOUND_TO_TRANSPARENT)
        {
            if (m_transparentCount >= BLIT_MAX_WORLD_TRANSPARENTS)
            {
                BLIT_ERROR("Exceeded max transparent transform count");
                return GCTransformCreateErrorCode;
            }

            BlitzenCore::BlitMemCopy(&m_transforms[CE_TRANSPARENT_OFFSET + m_transparentCount], context.m_pTransform, sizeof(MeshTransform));

            m_transformCount++;
            return CE_TRANSPARENT_OFFSET + m_transparentCount++;
        }

        // Function should have returned before this
        BLIT_ERROR("Unexpected transform type");
        return GCTransformCreateErrorCode;
    }

    void RandomizeTransform(MeshTransform* pTransform, float multiplier, float scale)
    {
        pTransform->pos = BlitML::vec3((float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier);

        pTransform->scale = scale;

        pTransform->orientation = BlitML::QuatFromAngleAxis(BlitML::vec3((float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1), BlitML::Radians((float(rand()) / RAND_MAX) * 90.f), 0);
    }

    void RandomizeTransform(WVTransform* pTransform, float multiplier)
    {
        pTransform->position = BlitML::vec3((float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier);
        pTransform->eulerAngles = BlitML::vec3((float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier);
    }
}