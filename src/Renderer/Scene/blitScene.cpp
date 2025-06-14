#include "blitScene.h"
#include "gltfScene.h"

namespace BlitzenEngine
{
    SCENE_CREATE_RES CreateScene(SceneContext* pScene, SCENE_CREATE_CONTEXT& sceneContext)
    {
        if (!sceneContext.pRenderer)
        {
            return SCENE_CREATE_RES::INVALID_RENDERER_HANDLE;
        }
        if (!sceneContext.pResidents)
        {
            return SCENE_CREATE_RES::INVALID_WORLD_RESIDENTS_HANDLE;
        }
        if (!sceneContext.pResources)
        {
            return SCENE_CREATE_RES::INVALID_RENDERING_RESOURCES_HANDLE;
        }

        constexpr float RENDERING_STRESS_TEST_RANDOM_TRANSFORM_MULTIPLIER = 3'000.f;

        switch (sceneContext.m_type)
        {
        case SceneType::GltfSceneTest:
        {
            auto res{ ManageGltf(sceneContext.m_name, sceneContext.pResources, sceneContext.pResidents, sceneContext.pRenderer, pScene) };
            BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL(res), "Fatal error encountered while loading gltf scene");
            return res;
        }
        case SceneType::RendererStressTest:
        {
            auto res{ LoadGeometryStressTest(sceneContext.pResidents, sceneContext.pResources, RENDERING_STRESS_TEST_RANDOM_TRANSFORM_MULTIPLIER, pScene) };
            BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL(res), "Fatal error encountered while loading renderer stress test scene");
            return res;
        }
        default:
        {
            return SCENE_CREATE_RES::UNKNOWN;
        }
        }
    }

    SCENE_CREATE_RES LoadGeometryStressTest(WORLD_RESIDENTS* pResidents, BlitzenEngine::RenderingResources* pResources, float transformMultiplier, BlitzenEngine::SceneContext* pScene)
    {
        constexpr uint32_t bunnyCount = 2'500'000;
        constexpr uint32_t kittenCount = 1'500'000;
        constexpr uint32_t maleCount = 90'000;
        constexpr uint32_t dragonCount = 10'000;
        constexpr uint32_t totalCount = bunnyCount + kittenCount + maleCount + dragonCount;

        constexpr float BunnyScale = 5.f;
        constexpr float KittenScale = 1.f;
        constexpr float HumanScale = 0.1f;
        constexpr float DragonScale = 0.2f;

        BLIT_WARN("Loading Renderer Stress test with %i objects", totalCount);

        pScene->m_name.CopyString("RendererStressTestScene");

        for (uint32_t i = 0; i < BlitzenCore::Ce_EngineDefaultMeshesCount; ++i)
        {
            pScene->m_meshNames.EmplaceEmtpy();
        }

        pScene->m_meshNames[0].CopyString(BlitzenCore::Ce_DefaultMeshName);
        pScene->m_meshNames[1].CopyString(BlitzenCore::Ce_DefaultKittenMeshName);
        pScene->m_meshNames[2].CopyString(BlitzenCore::Ce_DefaultDragonMeshName);
        pScene->m_meshNames[3].CopyString(BlitzenCore::Ce_DefaultHumanMeshname);

        pScene->m_renderOffset = pResidents->m_renders.m_renderCount;
        pScene->m_renderCount += bunnyCount + kittenCount + maleCount + dragonCount;

        uint32_t start = pResidents->m_renders.m_renderCount;

        // Bunnies
        for (uint32_t i = start; i < start + bunnyCount; ++i)
        {
            RESIDENT_CREATE_CONTEXT residentCtx{};
            residentCtx.m_flags = 0;
            residentCtx.m_pResource = &pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultMeshName];
            residentCtx.m_transformInfo.m_randomTransformMultiplier = transformMultiplier;
            residentCtx.m_transformInfo.m_scale = BunnyScale;

            auto bunnyRes{ pResidents->AddResident(residentCtx) };
            if (BlitzenCore::BLIT_CHECK_FAIL(bunnyRes))
            {
                BLIT_ERROR("Renderer Stress Test Scene-> Failed to create bunny residents");
                BlitzenCore::LOG_ERROR_MSG_AND_RETURN(BlitzenCore::CE_RESIDENT_SYSTEM_NAME, GET_RESIDENT_CREATE_RES_STRING(bunnyRes));
                return SCENE_CREATE_RES::SCENE_RESIDENTS_FAILURE;
            }
        }
        start += bunnyCount;

        // Kittens
        for (uint32_t i = start; i < start + kittenCount; ++i)
        {
            RESIDENT_CREATE_CONTEXT residentCtx{};
            residentCtx.m_flags = 0;
            residentCtx.m_pResource = &pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultKittenMeshName];
            residentCtx.m_transformInfo.m_randomTransformMultiplier = transformMultiplier;
            residentCtx.m_transformInfo.m_scale = KittenScale;

            auto kittenRes{ pResidents->AddResident(residentCtx) };
            if (BlitzenCore::BLIT_CHECK_FAIL(kittenRes))
            {
                BLIT_ERROR("Renderer Stress Test Scene-> Failed to create kitten residents");
                BlitzenCore::LOG_ERROR_MSG_AND_RETURN(BlitzenCore::CE_RESIDENT_SYSTEM_NAME, GET_RESIDENT_CREATE_RES_STRING(kittenRes));
                return SCENE_CREATE_RES::SCENE_RESIDENTS_FAILURE;
            }
        }
        start += kittenCount;

        // Standford dragons
        for (uint32_t i = start; i < start + dragonCount; ++i)
        {
            RESIDENT_CREATE_CONTEXT residentCtx{};
            residentCtx.m_flags = 0;
            residentCtx.m_pResource = &pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultDragonMeshName];
            residentCtx.m_transformInfo.m_randomTransformMultiplier = transformMultiplier;
            residentCtx.m_transformInfo.m_scale = DragonScale;

            auto dragonRes{ pResidents->AddResident(residentCtx) };
            if (BlitzenCore::BLIT_CHECK_FAIL(dragonRes))
            {
                BLIT_ERROR("Renderer Stress Test Scene-> Failed to create Dragon residents");
                BlitzenCore::LOG_ERROR_MSG_AND_RETURN(BlitzenCore::CE_RESIDENT_SYSTEM_NAME, GET_RESIDENT_CREATE_RES_STRING(dragonRes));
                return SCENE_CREATE_RES::SCENE_RESIDENTS_FAILURE;
            }
        }
        start += dragonCount;

        // Humans
        for (uint32_t i = start; i < start + maleCount; ++i)
        {
            RESIDENT_CREATE_CONTEXT residentCtx{};
            residentCtx.m_flags = 0;
            residentCtx.m_pResource = &pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultHumanMeshname];
            residentCtx.m_transformInfo.m_randomTransformMultiplier = transformMultiplier;
            residentCtx.m_transformInfo.m_scale = HumanScale;

            auto humanRes{ pResidents->AddResident(residentCtx) };
            if (BlitzenCore::BLIT_CHECK_FAIL(humanRes))
            {
                BLIT_ERROR("Renderer Stress Test Scene-> Failed to create kitten residents");
                BlitzenCore::LOG_ERROR_MSG_AND_RETURN(BlitzenCore::CE_RESIDENT_SYSTEM_NAME, GET_RESIDENT_CREATE_RES_STRING(humanRes));
                return SCENE_CREATE_RES::SCENE_RESIDENTS_FAILURE;
            }
        }

        return SCENE_CREATE_RES::SUCCESS;
    }
}