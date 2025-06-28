#include "blitScene.h"
#include "gltfScene.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "Renderer/WORLD/blitzenWorld.h"

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
		constexpr float MOVING_RESIDENT_TEST_RANDOM_TRANSFORM_MULTIPLIER = 100.f;

        switch (sceneContext.m_type)
        {
        case SceneType::GltfSceneTest:
        {
            auto res{ ManageGltf(sceneContext.m_name, sceneContext.pResources, sceneContext.pResidents, sceneContext.pRenderer, pScene) };
            BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)res), "Fatal error encountered while loading gltf scene");
            return res;
        }
        case SceneType::RendererStressTest:
        {
            auto res{ LoadGeometryStressTest(sceneContext.pResidents, sceneContext.pResources, RENDERING_STRESS_TEST_RANDOM_TRANSFORM_MULTIPLIER, pScene) };
            BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)res), "Fatal error encountered while loading renderer stress test scene");
            return res;
        }
        case SceneType::MovingResidentTest:
        {
            auto res{ LoadMovingResidentTest(sceneContext.pResidents, MOVING_RESIDENT_TEST_RANDOM_TRANSFORM_MULTIPLIER) };
            BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)res), "Fatal error encountered while loading moving resident test scene");
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
        constexpr float HumanScale = 0.2f;
        constexpr float DragonScale = 0.5f;

        BLIT_WARN("Loading Renderer Stress test with %i objects", totalCount);

        pScene->m_meshRefArr[0] = &pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultMeshName];
        pScene->m_meshRefArr[1] = &pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultKittenMeshName];
        pScene->m_meshRefArr[2] = &pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultDragonMeshName];
        pScene->m_meshRefArr[3] = &pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultHumanMeshname];

        uint32_t start = pResidents->m_renders.RENDER_COUNT;

        // Bunnies
        for (uint32_t i = start; i < start + bunnyCount; ++i)
        {
            RESIDENT_CREATE_CONTEXT residentCtx{};
            residentCtx.m_flags = 0;
            residentCtx.m_pResource = &pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultMeshName];

            // RandomizeTransform
            MeshTransform randomTransform;
            RandomizeTransform(&randomTransform, transformMultiplier, BunnyScale);
            residentCtx.m_transformInfo.m_pTransform = &randomTransform;

            RENDER_OBJECT_TYPE renderType{ RENDER_OBJECT_TYPE::OPAQUE_STATIC };
            residentCtx.m_isMoveable = BLIT_FAT_FALSE;

            auto bunnyRes{ pResidents->AddResident(residentCtx) };
            if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)bunnyRes))
            {
                BLIT_ERROR("%s: Renderer Stress Test Scene-> Failed to create bunny residents", BlitzenCore::CE_SCENE_SYSTEM_NAME);
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

            // Randomize transform
            MeshTransform randomTransform;
            RandomizeTransform(&randomTransform, transformMultiplier, KittenScale);
            residentCtx.m_transformInfo.m_pTransform = &randomTransform;

            RENDER_OBJECT_TYPE renderType{ RENDER_OBJECT_TYPE::OPAQUE_STATIC };
            residentCtx.m_isMoveable = BLIT_FAT_FALSE;

            auto kittenRes{ pResidents->AddResident(residentCtx) };
            if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)kittenRes))
            {
                BLIT_ERROR("%s: Renderer Stress Test Scene-> Failed to create kitten residents", BlitzenCore::CE_SCENE_SYSTEM_NAME);
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

            // Randomize transform
            MeshTransform randomTransform;
            RandomizeTransform(&randomTransform, transformMultiplier, DragonScale);
            residentCtx.m_transformInfo.m_pTransform = &randomTransform;

            RENDER_OBJECT_TYPE renderType{ RENDER_OBJECT_TYPE::OPAQUE_STATIC };
            residentCtx.m_isMoveable = BLIT_FAT_FALSE;

            auto dragonRes{ pResidents->AddResident(residentCtx) };
            if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)dragonRes))
            {
                BLIT_ERROR("%s: Renderer Stress Test Scene-> Failed to create Dragon residents", BlitzenCore::CE_SCENE_SYSTEM_NAME);
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

            // Randomize transform
            MeshTransform randomTransform;
            RandomizeTransform(&randomTransform, transformMultiplier, HumanScale);
            residentCtx.m_transformInfo.m_pTransform = &randomTransform;

            RENDER_OBJECT_TYPE renderType{ RENDER_OBJECT_TYPE::OPAQUE_STATIC };
            residentCtx.m_isMoveable = BLIT_FAT_FALSE;

            auto humanRes{ pResidents->AddResident(residentCtx) };
            if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)humanRes))
            {
                BLIT_ERROR("%s: Renderer Stress Test Scene-> Failed to create kitten residents", BlitzenCore::CE_SCENE_SYSTEM_NAME);
                BlitzenCore::LOG_ERROR_MSG_AND_RETURN(BlitzenCore::CE_RESIDENT_SYSTEM_NAME, GET_RESIDENT_CREATE_RES_STRING(humanRes));
                return SCENE_CREATE_RES::SCENE_RESIDENTS_FAILURE;
            }
        }

        return SCENE_CREATE_RES::SUCCESS;
    }

    static void RotatingKittenFunc(WORLD_VARIABLE worldVariable, float deltaTime)
    {
        constexpr float movementSpeed = 1.f;
        RotateEntity(worldVariable.m_engineResidentID, BlitML::fRotation{ 1.f }, deltaTime, BLIT_RESIDENT_MOVEMENT_ROTATING_PITCH_BIT | BLIT_RESIDENT_MOVEMENT_ROTATING_YAW_BIT | 
            BLIT_RESIDENT_MOVEMENT_ROTATING_ROLL_BIT);
    }

    SCENE_CREATE_RES LoadMovingResidentTest(WORLD_RESIDENTS* pResidents, float transformMultiplier)
    {
        constexpr uint32_t WV_ROTATING_KITTEN_COUNT = 5'000;
        constexpr float WV_ROTATING_KITTEN_SCALE = 1.f;

        for (uint32_t wv = 0; wv < WV_ROTATING_KITTEN_COUNT; ++wv)
        {
            WORLD_VARIABLE_CREATE_CONTEXT wvCtx{};
            wvCtx.residentCtx.m_flags = RESIDENT_CREATE_WORLD_VARIABLE;
            wvCtx.residentCtx.m_pResource = &RequestMeshResources_STATIC_ACCESS(BlitzenCore::Ce_DefaultKittenMeshName);

            RENDER_OBJECT_TYPE renderType = RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC;
            wvCtx.residentCtx.m_isMoveable = BLIT_FAT_TRUE;

            // Randomize Transform
            CPU_TRANSFORM randomTransform;
            RandomizeTransform(&randomTransform, transformMultiplier);
            wvCtx.residentCtx.m_transformInfo.cpu_pTransform = &randomTransform;

            MeshTransform randomTransform_gpu;
            RandomizeTransform(&randomTransform_gpu, transformMultiplier, WV_ROTATING_KITTEN_SCALE);
            wvCtx.residentCtx.m_transformInfo.m_pTransform = &randomTransform_gpu;

            wvCtx.residentCtx.m_transformInfo.m_type = WorldTransformType::DYNAMIC;

            wvCtx.m_worldVariableID = wv;

            auto movingRes{ pResidents->AddWorldVariable(wvCtx)};
            if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)movingRes))
            {
                BLIT_ERROR("%s: Moving Resident Test Scene-> FAILURE", BlitzenCore::CE_SCENE_SYSTEM_NAME);
                BlitzenCore::LOG_ERROR_MSG_AND_RETURN(BlitzenCore::CE_RESIDENT_SYSTEM_NAME, GET_RESIDENT_CREATE_RES_STRING(movingRes));
                return SCENE_CREATE_RES::SCENE_RESIDENTS_FAILURE;
            }

            BlitzenWorld::RegisterFrameEvent(pResidents->m_worldVariables[wv], RotatingKittenFunc);
        }
        return SCENE_CREATE_RES::SUCCESS;
    }
}