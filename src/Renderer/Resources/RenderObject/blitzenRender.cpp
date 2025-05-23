#include "blitRender.h"

namespace BlitzenEngine
{
    bool CreateRenderObject(RenderContainer& context, MeshResources& meshes, uint32_t transformId, uint32_t surfaceId)
    {
        // Create opaque render object
        if (!meshes.m_bTransparencyList[surfaceId].isTransparent)
        {
            if (context.m_renderCount >= BlitzenCore::Ce_MaxRenderObjects)
            {
                BLIT_ERROR("Max render object count reached");
                return false;
            }

            auto& render = context.m_renders[context.m_renderCount];
            render.surfaceId = surfaceId;
            render.transformId = transformId;

            context.m_renderCount++;
        }

        // Create transparent render object
        else
        {
            if (context.m_transparentRenderCount >= BlitzenCore::Ce_MaxTransparentRenderObjects)
            {
                BLIT_ERROR("Max transparent object count reached");
                return false;
            }

            auto& render = context.m_transparentRenders[context.m_transparentRenderCount];
            render.surfaceId = surfaceId;
            render.transformId = transformId;

            context.m_transparentRenderCount++;
        }

        return true;
    }

    uint32_t CreateRenderObjectFromMesh(RenderContainer& context, MeshResources& meshes, uint32_t meshId, const BlitzenEngine::MeshTransform& transform, bool isDynamic)
    {
        auto& mesh = meshes.m_meshes[meshId];
        if (context.m_renderCount + mesh.surfaceCount >= BlitzenCore::Ce_MaxRenderObjects)
        {
            BLIT_ERROR("Adding renderer objects from mesh will exceed the render object count");
            return BlitzenCore::Ce_MaxRenderObjects;
        }

        uint32_t transformId{ BlitzenCore::Ce_MaxRenderObjects };

        // Add to dynamic transforms
        if (isDynamic)
        {
            if (context.m_dynamicTransformCount >= BlitzenCore::Ce_MaxDynamicObjectCount)
            {
                BLIT_ERROR("Max dynamic mesh instance count reached");
                return BlitzenCore::Ce_MaxRenderObjects;
            }
            transformId = context.m_dynamicTransformCount;
            context.m_transforms[context.m_dynamicTransformCount++] = transform;
            context.m_transformCount++;
        }
        // Add to regular transforms
        else
        {
			if (context.m_staticTransformOffset >= BlitzenCore::Ce_MaxRenderObjects)
			{
				BLIT_ERROR("Max static mesh instance count reached");
				return BlitzenCore::Ce_MaxRenderObjects;
			}
            transformId = context.m_staticTransformOffset;
			context.m_transforms[context.m_staticTransformOffset++] = transform;
            context.m_transformCount++;
        }

        if (transformId == BlitzenCore::Ce_MaxRenderObjects)
        {
            BLIT_ERROR("Something went wrong when creating the render object");
            return transformId;
        }

        // Create render object for every surface in the mesh
        for (auto i = mesh.firstSurface; i < mesh.firstSurface + mesh.surfaceCount; ++i)
        {
            if (!CreateRenderObject(context, meshes, transformId, i))
            {
                BLIT_ERROR("Something went wrong when creating the render object");
                return BlitzenCore::Ce_MaxRenderObjects;
            }
        }

        return transformId;
    }

    void CreateSingleRender(RenderContainer& context, MeshResources& meshes, const char* meshName, float scale)
    {
        MeshTransform transform;

        transform.pos = BlitML::vec3(BlitzenCore::Ce_InitialCameraX, BlitzenCore::Ce_initialCameraY, BlitzenCore::Ce_initialCameraZ);
        transform.scale = scale;
        transform.orientation = BlitML::QuatFromAngleAxis(BlitML::vec3(0), 0, 0);

        CreateRenderObjectFromMesh(context, meshes, meshes.m_meshMap[meshName].meshId, transform, false);
    }

    void RandomizeTransform(MeshTransform& transform, float multiplier, float scale)
    {
        transform.pos = BlitML::vec3((float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier);

        transform.scale = scale;

        transform.orientation = BlitML::QuatFromAngleAxis(BlitML::vec3((float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1),
            BlitML::Radians((float(rand()) / RAND_MAX) * 90.f), 0);
    }

    void CreateRenderObjectWithRandomTransform(uint32_t meshId, RenderContainer& renders, MeshResources& meshContext, float randomTransformMultiplier, float scale)
    {
        // Creates a new transform, radomizes and creates render object based on it
        BlitzenEngine::MeshTransform transform;
        RandomizeTransform(transform, randomTransformMultiplier, scale);

		CreateRenderObjectFromMesh(renders, meshContext, meshId, transform, false);
    }

    // Creates the rendering stress test scene. 
    // TODO: This function is unsafe, calling it after another function that creates render object will cause issues
    void LoadGeometryStressTest(RenderContainer& renders, MeshResources& meshContext, float transformMultiplier)
    {
        // Don't load the stress test if ray tracing is on
#if defined(BLIT_VK_RAYTRACING)// The name of this macro should change
        return;
#endif

        constexpr uint32_t bunnyCount = 2'500'000;
        constexpr uint32_t kittenCount = 1'500'000;
        constexpr uint32_t maleCount = 90'000;
        constexpr uint32_t dragonCount = 10'000;
        constexpr uint32_t totalCount = bunnyCount + kittenCount + maleCount + dragonCount;

        BLIT_WARN("Loading Renderer Stress test with %i objects", totalCount);

        uint32_t start = renders.m_renderCount;

        // Bunnies
        for (uint32_t i = start; i < start + bunnyCount; ++i)
        {
            CreateRenderObjectWithRandomTransform(0, renders, meshContext, transformMultiplier, 5.f);
        }
        start += bunnyCount;
        // Kittens
        for (uint32_t i = start; i < start + kittenCount; ++i)
        {
            CreateRenderObjectWithRandomTransform(2, renders, meshContext, transformMultiplier, 1.f);
        }
        // Standford dragons
        start += kittenCount;
        for (uint32_t i = start; i < start + dragonCount; ++i)
        {
            CreateRenderObjectWithRandomTransform(1, renders, meshContext, transformMultiplier, 0.5f);
        }
        // Humans
        start += dragonCount;
        for (uint32_t i = start; i < start + maleCount; ++i)
        {
            CreateRenderObjectWithRandomTransform(3, renders, meshContext, transformMultiplier, 0.2f);
        }
    }

    // Creates a scene for oblique Near-Plane clipping testing. Pretty lackluster for the time being
    void CreateObliqueNearPlaneClippingTestObject(RenderContainer& renders, MeshResources& meshContext)
    {
        MeshTransform transform;
        transform.pos = BlitML::vec3(30.f, 50.f, 50.f);
        transform.scale = 2.f;
        transform.orientation = BlitML::QuatFromAngleAxis(BlitML::vec3(0), 0, 0);
        
		CreateRenderObjectFromMesh(renders, meshContext, meshContext.m_meshMap["kitten"].meshId, transform, false);

        const uint32_t nonReflectiveDrawCount = 1000;
        const uint32_t start = renders.m_renderCount;

        uint32_t meshId = meshContext.m_meshMap["kitten"].meshId;
        for (size_t i = start; i < start + nonReflectiveDrawCount; ++i)
        {
            CreateRenderObjectWithRandomTransform(meshId, renders, meshContext, 100.f, 1.f);
        }
    }
}