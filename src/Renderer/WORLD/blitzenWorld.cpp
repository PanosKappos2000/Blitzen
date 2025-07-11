#include "blitzenWorld.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/BlitzenWorld/blitzenUserInterface.h"

namespace BlitzenWorld
{
    inline WORLD_blit* p_BLITZEN_WORLD = nullptr;

    void WORLD_blit::DispatchFrameEvents(float deltaTime)
    {
        for (uint32_t event = 0; event < m_frameEvents.m_frameEventCount; ++event)
        {
            auto& frameEvent = m_frameEvents.m_frameEvents[event];
            frameEvent.m_function(frameEvent.m_worldVariableArg, deltaTime);
        }
    }

    void RegisterFrameEvent(BlitzenEngine::WORLD_VARIABLE worldVariable, BlitzenCore::FrameEventPfn function)
    {
        p_BLITZEN_WORLD->m_frameEvents.RegisterFrameEvent(worldVariable, function);
    }

    void RotateResidentAttachedCamera(BlitzenEngine::Resident resident, int32_t movementX, int32_t movementY)
    {
        BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < p_BLITZEN_WORLD->m_residents.m_transforms.m_moveableCount);

        auto& camera = p_BLITZEN_WORLD->m_cameras[p_BLITZEN_WORLD->m_activeCameraIDX];
        float deltaTime = p_BLITZEN_WORLD->deltaTime;

        float yaw = movementX > 0.f ? 0.5f : movementX < 0.f ? -0.5f : 0.f;
        float pitch = movementY > 0.f ? 0.5f : movementY < 0.f ? -0.5f : 0.f;

        constexpr float SavePitch = 89.f;
        
        camera.transformData.yawRotation += yaw * deltaTime;
        camera.transformData.pitchRotation += pitch * deltaTime;

        if (camera.transformData.pitchRotation > SavePitch)
        {
            camera.transformData.pitchRotation = SavePitch;
        }

        if (camera.attachmentSettings.attachmentFreeRotationFlag == BlitzenEngine::CAMERA_FREE_ROTATION_SETTING::ALWAYS ||
            (camera.attachmentSettings.attachmentFreeRotationFlag == BlitzenEngine::CAMERA_FREE_ROTATION_SETTING::NO_VELOCITY && BlitzenEngine::CheckResidentVelocity(resident) != 0.f))
        {
            BlitzenEngine::RotateEntity(resident, BlitML::fRotation(0.f, yaw, 0.f), deltaTime, BLIT_RESIDENT_MOVEMENT_ROTATING_YAW_BIT);
        }

        // New yaw pitch quat and rotation update
        auto yawOrientation = BlitML::QuatFromAngleAxis(BlitML::vec3(0.f, -1.f, 0.f), camera.transformData.yawRotation, 0);
        auto pitchOrientation = BlitML::QuatFromAngleAxis(BlitML::vec3(-1.f, 0.f, 0.f), camera.transformData.pitchRotation, 0);
        BlitzenEngine::CreateRotationMatrixFromPitchAndYawQuaternion(pitchOrientation, yawOrientation, camera.transformData.rotation);
    }

    void SetupCameraAttachment(uint32_t residentID, BlitML::float3 paddingFromAttachment, BlitzenEngine::CAMERA_FREE_ROTATION_SETTING freeRotationWhen)
    {
        auto& camera = p_BLITZEN_WORLD->m_cameras[p_BLITZEN_WORLD->m_activeCameraIDX];

        camera.attachmentSettings.attachmentID = residentID;
        camera.attachmentSettings.paddingFromAttachment = paddingFromAttachment;
        camera.attachmentSettings.attachmentFreeRotationFlag = freeRotationWhen;

        // The camera starts off at the position of the resident
        camera.viewData.position = BlitzenEngine::GetResidentPosition(residentID);

        // The camera starts off at the initial orientation of the resident
        camera.transformData.yawRotation = BlitzenEngine::GetResidentRotation(residentID).x;

		// Rotates the additional padding so that the camera is placed correctly even when the resident is rotated
        float offsetX = paddingFromAttachment.z * BlitML::Sin(camera.transformData.yawRotation);
		float offsetZ = paddingFromAttachment.z * BlitML::Cos(camera.transformData.yawRotation);
		auto finalPosition = camera.viewData.position + BlitML::vec3(offsetX, paddingFromAttachment.y, offsetZ);

        camera.transformData.translation = BlitML::Translate(finalPosition);

        auto yawOrientation = BlitML::QuatFromAngleAxis(BlitML::vec3{ 0.f, -1.f, 0.f }, camera.transformData.yawRotation, 0);
        auto pitchOrientation = BlitML::QuatFromAngleAxis(BlitML::vec3{ 1.f, 0.f, 0.f }, camera.transformData.pitchRotation, 0);

        // Combine for rotation
        BlitzenEngine::CreateRotationMatrixFromPitchAndYawQuaternion(pitchOrientation, yawOrientation, camera.transformData.rotation);

        // View matrix
        camera.viewData.viewMatrix = BlitML::Mat4Inverse(camera.transformData.translation * camera.transformData.rotation);
    }

    void MoveCameraReleased(BlitML::float3 movement)
    {
        auto& camera = p_BLITZEN_WORLD->m_cameras[p_BLITZEN_WORLD->m_activeCameraIDX];

        movement *= p_BLITZEN_WORLD->deltaTime * 20.f;
        auto directionalVelocity = camera.transformData.rotation * BlitML::vec4{ movement };
        camera.viewData.position = camera.viewData.position + BlitML::ToVec3(directionalVelocity);
    }

    void SNAP_MAIN()
    {
        auto& camera = p_BLITZEN_WORLD->m_cameras[p_BLITZEN_WORLD->m_activeCameraIDX];
        
        camera.transformData.translation = BlitML::Translate(p_BLITZEN_WORLD->m_residents.m_transforms.m_transforms[p_BLITZEN_WORLD->m_mainCharacter].pos);
    }

    bool CopyMeshResourcesToStagingBuffer(BlitzenEngine::MeshResources* pMeshes, BlitzenEngine::RenderingLoadingContextMesh& loadingContextMesh)
    {
        BlitzenEngine::HLSL_VTX_CONTEXT hlslVertices{};
        hlslVertices.m_vtxPosArr = pMeshes->m_triangles.m_vertexPositions;
        hlslVertices.m_vtxNrmArr = pMeshes->m_triangles.m_vertexNormals;
        hlslVertices.m_vtxTngArr = pMeshes->m_triangles.m_vertexTangents;
        hlslVertices.m_texCoordArr = pMeshes->m_triangles.m_vertexUVs;
        BlitzenEngine::ConvertClassicVerticesToHlslFormat(hlslVertices, pMeshes->m_triangles.m_vertices, pMeshes->m_triangles.m_vertexCount);

        if (!BlitzenEngine::UploadToVertexPositionsStagingBuffer(loadingContextMesh, pMeshes->m_triangles.m_vertexPositions, pMeshes->m_triangles.m_vertexCount))
        {
            BLIT_ERROR("%s: Failed to upload vertex positions to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        if (!BlitzenEngine::UploadToVertexNormalsStagingBuffer(loadingContextMesh, pMeshes->m_triangles.m_vertexNormals, pMeshes->m_triangles.m_vertexCount))
        {
            BLIT_ERROR("%s: Failed to upload vertex normals to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        if (!BlitzenEngine::UploadToVertexTangentsStagingBuffer(loadingContextMesh, pMeshes->m_triangles.m_vertexTangents, pMeshes->m_triangles.m_vertexCount))
        {
            BLIT_ERROR("%s: Failed to upload vertex tangents to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        if (!BlitzenEngine::UploadToVertexTextureCoordinatesStagingBuffer(loadingContextMesh, pMeshes->m_triangles.m_vertexUVs, pMeshes->m_triangles.m_vertexCount))
        {
            BLIT_ERROR("%s: Failed to upload vertex texture coordinates to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        if (!BlitzenEngine::UploadToVertexIndicesStagingBuffer(loadingContextMesh, pMeshes->m_triangles.m_indices, pMeshes->m_triangles.m_vtxIdxCount))
        {
            BLIT_ERROR("%s: Failed to upload vertex indices to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        pMeshes->m_triangles.m_mapIdxCount += pMeshes->m_triangles.m_vtxIdxCount;
        pMeshes->m_triangles.m_vtxIdxCount = 0;
        pMeshes->m_triangles.m_mapVtxCount += pMeshes->m_triangles.m_vertexCount;
        pMeshes->m_triangles.m_vertexCount = 0;

        // success
        return true;
    }

    bool RenderingResourcesInit(BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer, BlitzenEngine::RenderingLoadingContextMesh& loadingContextMesh)
    {
        pResources->m_textureManager.ALLOC();
        if (!BlitzenEngine::UploadTextureToGPU(pRenderer, pResources->m_textureManager.m_singleTextureHandle, "Assets/Textures/BlitzenLSV1.dds"))
        {
            BLIT_ERROR("%s: Rendering resources failed", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        if (!BlitzenEngine::UploadRendererIdleWorkResources(pRenderer, BlitzenEngine::RENDERER_IDLE_MODE::BLITZEN_LOGO))
        {
            BLIT_ERROR("%s: Failed to put renderer on Idle Work Mode", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        // Does not return false by design, might change later.
        if (!pResources->m_textureManager.AddTexture(BlitzenCore::Ce_DefaultTextureName))
        {
            BLIT_ERROR("%s: Something went wrong with texture map", BlitzenCore::CE_WORLD_SYSTEM_NAME);
        }

        if (!pResources->m_textureManager.AddMaterial(0, 0, 0, 0, BlitzenCore::Ce_DefaultMaterialName))
        {
            BLIT_ERROR("Rendering resources failed");
            return false;
        }

        pResources->m_meshContext.m_triangles.ALLOC();
        pResources->m_meshContext.m_clusters.ALLOC();

#if defined(CUSTOM_FILE_TEST) && !defined(MOVING_RESIDENT_TEST) && !defined(DEFAULT_GLTF_SCENE_TEST) && !defined(LOAD_CMD_ARG_GLTF_FILEPATHS) && !defined(RENDERER_STRESS_TEST)
        // Skip hardcoded load
#else
        BlitzenEngine::AllocateLoadingContextMesh(pRenderer, loadingContextMesh);

        uint32_t bunnyMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/bunny.obj", BlitzenCore::Ce_DefaultMeshName) };
        if (bunnyMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default bunny mesh");
            return false;
        }

        if (!CopyMeshResourcesToStagingBuffer(&pResources->m_meshContext, loadingContextMesh))
        {
            BLIT_ERROR("%s: Failed to copy mesh resources to staging buffer for bunny mesh", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        uint32_t kittenMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/kitten.obj", BlitzenCore::Ce_DefaultKittenMeshName) };
        if (kittenMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default kitten mesh");
            return false;
        }

        if (!CopyMeshResourcesToStagingBuffer(&pResources->m_meshContext, loadingContextMesh))
        {
            BLIT_ERROR("%s: Failed to copy mesh resources to staging buffer for kitten mesh", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        uint32_t dragonMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/dragon.obj", BlitzenCore::Ce_DefaultDragonMeshName) };
        if (dragonMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default dragon mesh");
            return false;
        }

        if (!CopyMeshResourcesToStagingBuffer(&pResources->m_meshContext, loadingContextMesh))
        {
            BLIT_ERROR("%s: Failed to copy mesh resources to staging buffer for dragon mesh", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        uint32_t humanMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/FinalBaseMesh.obj", BlitzenCore::Ce_DefaultHumanMeshname) };
        if (humanMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default human mesh");
            return false;
        }

        if (!CopyMeshResourcesToStagingBuffer(&pResources->m_meshContext, loadingContextMesh))
        {
            BLIT_ERROR("%s: Failed to copy mesh resources to staging buffer for human mesh", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }
#endif

        pResources->m_terrainContainer.ALLOC();

        BlitzenEngine::InitializeMeshResourcesPointer_STATIC_ACCESS(&pResources->m_meshContext);

        // Success
        return true;
    }

    void LOAD_RESOURCES_MK_BLIT_MINUS(WORLD_blit* pWORLD, BlitzenEngine::RenderingResources* pRenderingResources, BlitzenEngine::RenderingLoadingContextMesh& loadingContextMesh,
        int argc, char** argv)
    {

        BlitzenEngine::SCENE_CREATE_CONTEXT sceneCtx{};
        sceneCtx.pRenderer = pWORLD->P_RENDERER.Data();
        sceneCtx.pResidents = &pWORLD->m_residents;
        sceneCtx.pResources = pRenderingResources;

        BlitCL::DynamicArray<BlitzenEngine::SceneContext> scenes{};

#if defined(CUSTOM_FILE_TEST) && !defined(MOVING_RESIDENT_TEST) && !defined(DEFAULT_GLTF_SCENE_TEST) && !defined(LOAD_CMD_ARG_GLTF_FILEPATHS) && !defined(RENDERER_STRESS_TEST)

        BlitzenEngine::SceneContext rpf{};
        rpf.m_type = BlitzenEngine::SceneType::CustomFileTest;
        scenes.PushBack(rpf);

#endif

#if defined(RENDERER_STRESS_TEST)

        BlitzenEngine::SceneContext stress{};
        stress.m_type = BlitzenEngine::SceneType::RendererStressTest;
        scenes.PushBack(stress);

#endif

        BlitzenEngine::SceneContext moving{};
        moving.m_type = BlitzenEngine::SceneType::MovingResidentTest;
        scenes.PushBack(moving);

#if defined(DEFAULT_GLTF_SCENE_TEST)

        BlitzenEngine::SceneContext defaultGltf{};
        defaultGltf.m_name = BlitzenCore::Ce_PrimaryGltfTestScene;
        defaultGltf.m_type = BlitzenEngine::SceneType::GltfSceneTest;
        scenes.PushBack(defaultGltf);

#endif

#if defined(LOAD_CMD_ARG_GLTF_FILEPATHS)

        if (argc > 1)
        {
            BlitzenEngine::SceneContext defaultGltf{};
            defaultGltf.m_name = argv[1];
            defaultGltf.m_type = BlitzenEngine::SceneType::GltfSceneTest;
            scenes.PushBack(defaultGltf);
        }
#endif

#if defined(COLLISION_TEST)

        BlitzenEngine::SceneContext collisionTst{};
        collisionTst.m_type = BlitzenEngine::SceneType::SmallSceneForCollision;
        scenes.PushBack(collisionTst);

#endif
        sceneCtx.m_sceneArr = scenes.Data();
        sceneCtx.m_sceneCount = (uint32_t)scenes.GetSize();

        auto sceneRes{ BlitzenEngine::CreateScene(sceneCtx, loadingContextMesh) };

        BLIT_ASSERT(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)sceneRes));
        if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)sceneRes))
        {
            BLIT_ERROR("%s: Error while loading scenes. Received error message: %s", BlitzenCore::CE_BLITZEN_LOADING_LOOP_NAME, BlitzenEngine::GET_SCENE_CREATE_RES_STRING(sceneRes));
            BLIT_ASSERT(false);
        }

        pWORLD->m_collisionGrid.DefineGrid(0);
        pWORLD->m_collisionGrid.CreateCells();
        pWORLD->m_collisionGrid.PlaceStatics(&pWORLD->m_residents.m_renders.m_renders[BLIT_OPAQUE_STATIC_RENDER_OFFSET], pWORLD->m_residents.m_transforms.m_staticTransformCount,
            pWORLD->m_residents.m_transforms.m_transforms);

        BLIT_ASSERT(BlitGenerator::GenerateTerrainVertices(pRenderingResources->m_terrainContainer));

        //BlitzenEngine::MESH_PRIMITIVE_CREATE_CONTEXT meshPrimitiveCtx{};
        //meshPrimitiveCtx.m_indexCount = pRenderingResources->m_terrainContainer.terrainIndexCount;
        //meshPrimitiveCtx.m_indices = pRenderingResources->m_terrainContainer.terrainIndices;
        //meshPrimitiveCtx.m_materialID = 0;
        //meshPrimitiveCtx.m_vertexCount = pRenderingResources->m_terrainContainer.terrainVertexCount;
        //meshPrimitiveCtx.m_vertices = pRenderingResources->m_terrainContainer.terrainVertices;
        //pRenderingResources->m_meshContext.m_meshPrimitives.GenerateSurface(pRenderingResources->m_meshContext.m_triangles, pRenderingResources->m_meshContext.m_clusters, meshPrimitiveCtx);

#if defined(CUSTOM_FILE_TEST) && !defined(MOVING_RESIDENT_TEST) && !defined(DEFAULT_GLTF_SCENE_TEST) && !defined(LOAD_CMD_ARG_GLTF_FILEPATHS) && !defined(RENDERER_STRESS_TEST)

#else

        if (!BlitzenEngine::UploadResourcesToGPU(pWORLD->P_RENDERER.Data(), pWORLD->m_drawContext, loadingContextMesh))
        {
            BLIT_FATAL("Renderer failed to setup, Blitzen shutting down");
            BLIT_ASSERT(false);
            return;
        }

#endif
        pRenderingResources->m_meshContext.m_triangles.CLEAN();
        pRenderingResources->m_meshContext.m_clusters.CLEAN();
    }

    void INITIALIZE_WORLD_POINTER(WORLD_blit* ptr)
    {
        BLIT_ASSERT_MESSAGE(p_BLITZEN_WORLD == nullptr, "Tried to reinitialize WORLD pointer");
        p_BLITZEN_WORLD = ptr;
    }
}