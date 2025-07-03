#include "blitScene.h"
#include "gltfScene.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "Renderer/WORLD/blitzenWorld.h"
#include "Core/BlitzenWorld/blitzenUserInterface.h"
#include "Core/Events/blitEvents.h"

namespace BlitzenEngine
{
    SCENE_CREATE_RES CreateScene(SCENE_CREATE_CONTEXT& sceneContext)
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
        constexpr float CUSTOM_FILE_TEST_RANDOM_TRANSFORM_MULTIPLIER = 2'000.f;
        constexpr float COLLISION_TEST_MULTIPLIER = 2'000.f;

        for (uint32_t ctx = 0; ctx < sceneContext.m_sceneCount; ++ctx)
        {
            auto& scene = sceneContext.m_sceneArr[ctx];
            switch (scene.m_type)
            {
            case SceneType::GltfSceneTest:
            {
                auto res{ ManageGltf(scene.m_name, sceneContext.pResources, sceneContext.pResidents, sceneContext.pRenderer, &scene) };
                BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)res), "Fatal error encountered while loading gltf scene");
                if (BlitzenCore::BLIT_CHECK_FAIL(int64_t(res)))
                {
                    BLIT_ERROR("%s: Failed to create gltf scene", BlitzenCore::CE_SCENE_SYSTEM_NAME);
                    return res;
                }
                break;
            }
            case SceneType::RendererStressTest:
            {
                auto res{ LoadGeometryStressTest(sceneContext.pResidents, sceneContext.pResources, RENDERING_STRESS_TEST_RANDOM_TRANSFORM_MULTIPLIER, &scene) };
                BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)res), "Fatal error encountered while loading renderer stress test scene");
                if (BlitzenCore::BLIT_CHECK_FAIL(int64_t(res)))
                {
                    BLIT_ERROR("%s: Failed to create renering stress test scene", BlitzenCore::CE_SCENE_SYSTEM_NAME);
                    return res;
                }
                break;
            }
            case SceneType::MovingResidentTest:
            {
                auto res{ LoadMovingResidentTest(sceneContext.pResidents, MOVING_RESIDENT_TEST_RANDOM_TRANSFORM_MULTIPLIER) };
                BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)res), "Fatal error encountered while loading moving resident test scene");
                if (BlitzenCore::BLIT_CHECK_FAIL(int64_t(res)))
                {
                    BLIT_ERROR("%s: Failed to create moving resident test scene", BlitzenCore::CE_SCENE_SYSTEM_NAME);
                    return res;
                }
                break;
            }
            case SceneType::CustomFileTest:
            {
                auto res{ LoadCustomFileTest(sceneContext.pResidents, sceneContext.pRenderer, sceneContext.pResources, CUSTOM_FILE_TEST_RANDOM_TRANSFORM_MULTIPLIER) };
                BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL(int64_t(res)), "Fatal error encountered while loading custom file stress test scene");
                if (BlitzenCore::BLIT_CHECK_FAIL(int64_t(res)))
                {
                    BLIT_ERROR("%s: Failed to create custom file (rpf) test scene", BlitzenCore::CE_SCENE_SYSTEM_NAME);
                    return res;
                }
                break;
            }
            case SceneType::SmallSceneForCollision:
            {
                auto res{ LoadCollisionTest(sceneContext.pResidents, sceneContext.pResources, COLLISION_TEST_MULTIPLIER, &scene) };
                BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL(int64_t(res)), "Fatal error encountered while loading small scene for collision testing");
                if (BlitzenCore::BLIT_CHECK_FAIL(int64_t(res)))
                {
                    BLIT_ERROR("%s: Failed to create small scene for collision testing", BlitzenCore::CE_SCENE_SYSTEM_NAME);
                    return res;
                }
                break;
            }
            default:
            {
                return SCENE_CREATE_RES::UNKNOWN;
            }
            }
        }

        return SCENE_CREATE_RES::SUCCESS;
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
            residentCtx.m_resourceID = pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultMeshName].firstSurface;

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
            residentCtx.m_resourceID = pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultKittenMeshName].firstSurface;

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
            residentCtx.m_resourceID = pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultDragonMeshName].firstSurface;

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
            residentCtx.m_resourceID = pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultHumanMeshname].firstSurface;

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

    SCENE_CREATE_RES LoadCollisionTest(WORLD_RESIDENTS* pResidents, BlitzenEngine::RenderingResources* pResources, float transformMultiplier, BlitzenEngine::SceneContext* pScene)
    {
        constexpr uint32_t kittenCount = 150'000;
        
        constexpr float KittenScale = 1.f;

        // Kittens
        for (uint32_t i = 0; i < kittenCount; ++i)
        {
            RESIDENT_CREATE_CONTEXT residentCtx{};
            residentCtx.m_flags = 0;
            residentCtx.m_resourceID = pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultKittenMeshName].firstSurface;

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
            wvCtx.residentCtx.m_resourceID = RequestMeshResources_STATIC_ACCESS(BlitzenCore::Ce_DefaultKittenMeshName).firstSurface;

            RENDER_OBJECT_TYPE renderType = RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC;
            wvCtx.residentCtx.m_isMoveable = BLIT_FAT_TRUE;

            // Randomize Transform
            CPU_TRANSFORM randomTransform;
            RandomizeTransform(&randomTransform, transformMultiplier);
            wvCtx.residentCtx.m_transformInfo.cpu_pTransform = &randomTransform;
            wvCtx.residentCtx.m_transformInfo.cpu_pTransform->movementFlags |= BLIT_RESIDENT_MOVEMENT_GRAVITY_BIT;

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

            if (wv == 0)
            {
                //BlitzenCore::RegisterEvent()
            }
            else
            {
                BlitzenWorld::RegisterFrameEvent(pResidents->m_worldVariables[wv], RotatingKittenFunc);
            }
        }
        return SCENE_CREATE_RES::SUCCESS;
    }

    SCENE_CREATE_RES LoadCustomFileTest(WORLD_RESIDENTS* pResidents, RendererPtrType pRenderer, BlitzenEngine::RenderingResources* pResources, float transformMultiplier)
    {
        auto pMeshContainer{ &pResources->m_meshContext };

        uint32_t meshID = LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/kitten.obj", BlitzenCore::Ce_DefaultKittenMeshName);
        if (meshID == BlitzenCore::Ce_MaxMeshCount)
        {
            return SCENE_CREATE_RES::FAILED_TO_LOAD_OBJ_MESH_TO_DISK;
        }
        
        RenderingLoadingContextMesh loadingContextMesh{};
        if (!AllocateLoadingContextMesh(pRenderer, loadingContextMesh))
        {
            return SCENE_CREATE_RES::FAILED_TO_ALLOCATE_RENDERER_STAGING_BUFFERS;
        }

        if (!UploadToMeshPrimitiveStagingBuffer(loadingContextMesh, pMeshContainer->m_meshPrimitives.m_meshPrimitives, pMeshContainer->m_meshPrimitives.m_meshPrimitivesCount))
        {
            return SCENE_CREATE_RES::FAILED_TO_UPLOAD_MESH_PRIMITIVES_TO_STAGING_BUFFER;
        }

        if (!UploadToLODDataStagingBuffer(loadingContextMesh, pMeshContainer->m_meshPrimitives.m_LODs, pMeshContainer->m_meshPrimitives.m_LODCount))
        {
            return SCENE_CREATE_RES::FAILED_TO_UPLOAD_LOD_DATA_TO_STAGING_BUFFER;
        }

        if (!UploadToVertexPositionsStagingBuffer(loadingContextMesh, pMeshContainer->m_triangles.m_vertexPositions, pMeshContainer->m_triangles.m_vertexCount))
        {
            return SCENE_CREATE_RES::FAILED_TO_UPLOAD_VERTEX_POSITIONS_TO_STAGING_BUFFER;
        }

        if (!UploadToVertexNormalsStagingBuffer(loadingContextMesh, pMeshContainer->m_triangles.m_vertexNormals, pMeshContainer->m_triangles.m_vertexCount))
        {
            return SCENE_CREATE_RES::FAILED_TO_UPLOAD_VERTEX_NORMALS_TO_STAGING_BUFFER;
        }

        if (!UploadToVertexTangentsStagingBuffer(loadingContextMesh, pMeshContainer->m_triangles.m_vertexTangents, pMeshContainer->m_triangles.m_vertexCount))
        {
            return SCENE_CREATE_RES::FAILED_TO_UPLOAD_VERTEX_TANGENTS_TO_STAGING_BUFFER;
        }

        if (!UploadToVertexTextureCoordinatesStagingBuffer(loadingContextMesh, pMeshContainer->m_triangles.m_vertexUVs, pMeshContainer->m_triangles.m_vertexCount))
        {
            return SCENE_CREATE_RES::FAILED_TO_UPLOAD_VERTEX_TEXTURE_COORDINATES_TO_STAGING_BUFFER;
        }

        if (!UploadToVertexIndicesStagingBuffer(loadingContextMesh, pMeshContainer->m_triangles.m_indices, pMeshContainer->m_triangles.m_vtxIdxCount))
        {
            return SCENE_CREATE_RES::FAILED_TO_UPLOAD_VERTEX_INDICES_TO_STAGING_BUFFER;
        }

        constexpr uint32_t kittenCount = 1'500'000;
        constexpr float KittenScale = 1.f;

        RESIDENT_CREATE_CONTEXT residentCtx{};
        residentCtx.m_flags = 0;
        residentCtx.m_resourceID = pResources->m_meshContext.m_meshMap[BlitzenCore::Ce_DefaultKittenMeshName].firstSurface;

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

        return SCENE_CREATE_RES::SUCCESS;
    }
}