#include "blitzenWorld.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/BlitzenWorld/blitzenUserInterface.h"

namespace BlitzenWorld
{
    inline BLITZEN_WORLD* GSBlitzenWorld = nullptr;

    void BLITZEN_WORLD::DispatchFrameEvents(float deltaTime)
    {
        for (uint32_t event = 0; event < m_frameEvents.m_frameEventCount; ++event)
        {
            auto& frameEvent = m_frameEvents.m_frameEvents[event];
            frameEvent.m_function(frameEvent.m_resident, deltaTime);
        }
    }

    void DispatchCollisionSystems(BLITZEN_WORLD* pWORLD)
    {
        BlitzenCore::BlitPerformanceCounter counter;
        counter.GenerateInner();
        double broadPhaseEnd = 0;
        double broadPhaseState = counter.Startup();
        BlitzenEngine::COLLISION_RESOLVE_CONTEXT collisionResolveContext{};
        collisionResolveContext.WVTransformArr = pWORLD->mResidents.WVTransforms;
        collisionResolveContext.mTransformCount = pWORLD->mResidents.mWorldVariableCount;
        pWORLD->mResidents.MColliders.DispatchCollisionResolve(&pWORLD->mCollisionGrid, collisionResolveContext);
        broadPhaseEnd = counter.End();
        counter.Reset();
    }

    void DispatchBumper(BlitzenWorld::BLITZEN_WORLD* WORLD, uint32_t terrainCount)
    {
        auto pRenderer = WORLD->BMPR.Data();
        auto& camera = WORLD->m_cameras[WORLD->m_activeCameraIDX];

        // To avoid the double fence, I have to split the camera data.
        BlitzenEngine::PlaceRendererFence(pRenderer, BlitzenEngine::RENDERER_FENCE_TYPE::COMPUTE);
        BlitzenEngine::PlaceRendererFence(pRenderer, BlitzenEngine::RENDERER_FENCE_TYPE::GRAPHICS);

        // Passes camera values to GPU
        BlitzenEngine::UpdateRendererView(pRenderer, camera.viewData, camera.transformData.bFreezeFrustum);

        // Starts with Hierarchical Z Buffer, built with previous frame depth target values
        // Compute queue has started recording at this point
        BlitzenEngine::BeginGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::COMPUTE);
        BlitzenEngine::GenerateHI_Z_MAP(pRenderer);

        // Commands already recording for HI Z. Now Setting general descriptors
        BlitzenEngine::BindGeneralComputeDescriptors(pRenderer);

        // Static draws do not have to wait for anything else. Culling begins
        BlitzenEngine::CULL_CONTEXT cullContext{};
        cullContext.m_cullType = BlitzenEngine::BLIT_CULL_TYPE::DRAW_CULL_TEMPORAL_OCCLUSION;
        cullContext.m_workType = BlitzenEngine::RENDER_OBJECT_TYPE::OPAQUE_STATIC;
        cullContext.m_workCount = WORLD->mResidents.m_renders.m_opaqueStaticCount;
        cullContext.m_pResidents = &WORLD->mResidents;// IS THIS NEEDED?
        BlitzenEngine::DispatchCullingShaders(pRenderer, cullContext);
        // End compute commands here, so that the fence can be signaled
        BlitzenEngine::EndGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::COMPUTE);

        // Begins graphics commands and starts the render pass (clears color buffer)
        BlitzenEngine::BeginGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::GRAPHICS);
        BlitzenEngine::SetupForFirstRenderPass(pRenderer);
        BlitzenEngine::RenderTerrain(pRenderer, terrainCount);

        // Waits for static object culling (compute shader0
        BlitzenEngine::PlaceRendererFence(pRenderer, BlitzenEngine::RENDERER_FENCE_TYPE::COMPUTE);

        // Now draws static render objects
        BlitzenEngine::RENDER_CONTEXT staticRenderContext{};
        staticRenderContext.m_renderType = BlitzenEngine::BLIT_RENDER_TYPE::RENDER_OPAQUE;
        BlitzenEngine::RenderObjects(pRenderer, staticRenderContext);

        // Starts transfer commands. The function blocks the culling shader, until the dynamic transforms are updated
        BlitzenEngine::BeginGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::TRANSFER);
        BlitzenEngine::UpdateRendererTransforms(pRenderer, WORLD->mResidents.WVTransforms, WORLD->mResidents.mWorldVariableCount);

        // Start transforming and culling dynamic objects
        BlitzenEngine::BeginGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::COMPUTE);
        BlitzenEngine::BindGeneralComputeDescriptors(pRenderer);
        cullContext.m_cullType = BlitzenEngine::BLIT_CULL_TYPE::DRAW_CULL_TEMPORAL_OCCLUSION;
        cullContext.m_workType = BlitzenEngine::RENDER_OBJECT_TYPE::OPAQUE_DYNAMIC;
        cullContext.m_workCount = WORLD->mResidents.m_renders.m_opaqueDynamicCount;
        BlitzenEngine::DispatchCullingShaders(pRenderer, cullContext);

        if constexpr (BLITGCNarrowPhaseCollisionBumper)
        {

        }
        else if constexpr (BLITGCBroadPhaseCollisionBumper)
        {
            BlitzenEngine::BMPRDispatchBroadPhaseCollision(pRenderer, &WORLD->MBmprCollisionWorkConstant);
        }

        // Puts buffers that must be readback after the shader into readback mode and blocks transfer
        // Also blocks graphics until culling shader is done
        BlitzenEngine::ChangeCullingBuffersToReadbackMode(pRenderer);
        BlitzenEngine::EndGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::COMPUTE);
        BlitzenEngine::PlaceRendererFence(pRenderer, BlitzenEngine::RENDERER_FENCE_TYPE::COMPUTE);

        // Dynamic object graphics
        BlitzenEngine::RENDER_CONTEXT dynamicRenderContext{};
        dynamicRenderContext.m_renderType = BlitzenEngine::BLIT_RENDER_TYPE::RENDER_DYNAMIC;
        BlitzenEngine::RenderObjects(pRenderer, dynamicRenderContext);
        BlitzenEngine::FinalizeRendering(pRenderer);
        BlitzenEngine::EndGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::GRAPHICS);

        // Pass data back to the CPU for logic updates
        BlitzenEngine::SHADER_GAME_LOGIC_UPDATES shaderDataReadback{};
        shaderDataReadback.m_transformCount = WORLD->mResidents.mWorldVariableCount;
        shaderDataReadback.pGpuTransorms = WORLD->mResidents.WVTransforms;
        BlitzenEngine::RequestGameLogicUpdatesFromShader(pRenderer, shaderDataReadback);
        BlitzenEngine::EndGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::COMPUTE);
    }

    void RegisterFrameEvent(BlitzenEngine::WORLD_VARIABLE worldVariable, BlitzenCore::FrameEventPfn function)
    {
        GSBlitzenWorld->m_frameEvents.RegisterFrameEvent(worldVariable, function);
    }

    void RequestGameCameraRotation(BlitzenEngine::Resident resident, int32_t movementX, int32_t movementY)
    {
        BLIT_RUNTIME_TEST_CHECK_VOID_RETURN(resident < GSBlitzenWorld->mResidents.mWorldVariableCount);

        auto& camera = GSBlitzenWorld->m_cameras[GSBlitzenWorld->m_activeCameraIDX];
        float deltaTime = GSBlitzenWorld->deltaTime;

        constexpr float CameraSensitivity = 0.1f;// To be placed in settings struct
        constexpr int32_t CameraDeadzone = 1;
        constexpr float FineRadians = 0.0001f;

        // Filter out small mouse movement
        if (abs(movementX) < CameraDeadzone)
        {
            movementX = 0;
        }
        if (abs(movementY) < CameraDeadzone)
        {
            movementY = 0;
        }

        // Do not create radians for 0 movement
        if (movementX != 0)
        {
            camera.transformData.yawMovement = BlitML::Radians((float)movementX * CameraSensitivity);
        }
        else
        {
            camera.transformData.yawMovement = 0.f;
        }
        if (movementY != 0)
        {
            camera.transformData.pitchMovement = BlitML::Radians((float)movementY * CameraSensitivity);
        }
        else
        {
            camera.transformData.pitchMovement = 0.f;
        }

        if (fabs(camera.transformData.pitchMovement) <= FineRadians)
        {
            camera.transformData.pitchMovement = 0.f;
        }
        if (fabs(camera.transformData.yawMovement) <= FineRadians)
        {
            camera.transformData.yawMovement = 0.f;
        }

        camera.transformData.yawRotation += camera.transformData.yawMovement;
        camera.transformData.pitchRotation += camera.transformData.pitchMovement;
    }

    void SetupCameraAttachment(uint32_t residentID, BlitML::float3 paddingFromAttachment, BlitzenEngine::CAMERA_FREE_ROTATION_SETTING freeRotationWhen)
    {
        auto& camera = GSBlitzenWorld->m_cameras[GSBlitzenWorld->m_activeCameraIDX];

        camera.attachmentSettings.attachmentID = residentID;
        camera.attachmentSettings.paddingFromAttachment = paddingFromAttachment;
        camera.attachmentSettings.attachmentFreeRotationFlag = freeRotationWhen;

        // The camera starts off at the position of the resident
        camera.viewData.position = BlitzenEngine::GetResidentPosition(residentID);

        // Fixes player orientation
        GSBlitzenWorld->mResidents.WVTransforms[residentID].eulerAngles = BlitML::fRotation(camera.transformData.pitchRotation, camera.transformData.yawRotation, 0.f);
        BlitML::quat orientationYaw = BlitML::NormalizedQuatFromAngleAxis(BlitML::float3(0.f, -1.f, 0.f), GSBlitzenWorld->mResidents.WVTransforms[residentID].eulerAngles.x);
        BlitML::quat orientationPitch = BlitML::NormalizedQuatFromAngleAxis(BlitML::float3(1.f, 0.f, 0.f), GSBlitzenWorld->mResidents.WVTransforms[residentID].eulerAngles.y);
        GSBlitzenWorld->mResidents.mTransforms.m_transforms[residentID].orientation = BlitML::MulitplyQuat(orientationYaw, orientationPitch);

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
        auto& camera = GSBlitzenWorld->m_cameras[GSBlitzenWorld->m_activeCameraIDX];

        movement *= GSBlitzenWorld->deltaTime * 20.f;
        auto directionalVelocity = camera.transformData.rotation * BlitML::vec4{ movement };
        camera.viewData.position = camera.viewData.position + BlitML::ToVec3(directionalVelocity);
    }

    void SNAP_MAIN()
    {
        auto& camera = GSBlitzenWorld->m_cameras[GSBlitzenWorld->m_activeCameraIDX];
        
        camera.transformData.translation = BlitML::Translate(GSBlitzenWorld->mResidents.mTransforms.m_transforms[GSBlitzenWorld->m_mainCharacter].pos);
    }

    uint32_t GetCurrentWorldVariableCount()
    {
        return GSBlitzenWorld->mResidents.mWorldVariableCount;
    }

    uint32_t GetCurrentColliderCount()
    {
        return GSBlitzenWorld->mResidents.MColliders.mStaticColliderCount + BLIT_MAX_WORLD_VARIABLE_COUNT;
    }

    uint32_t GetStaticColliderCount()
    {
        return GSBlitzenWorld->mResidents.MColliders.mStaticColliderCount;
    }

    uint32_t GetCurrentWorldVariableColliderCount()
    {
        return GSBlitzenWorld->mResidents.MColliders.mWorldVariableColliderCount;
    }

    uint32_t GetCurrentTransformCount()
    {
        return GSBlitzenWorld->mResidents.mTransforms.m_staticTransformCount + BLIT_MAX_WORLD_VARIABLE_COUNT;
    }

    uint32_t GetStaticTransformCount()
    {
        return GSBlitzenWorld->mResidents.mTransforms.m_staticTransformCount;
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
        BlitzenEngine::InitializeTerrainContainerPtr(&pResources->m_terrainContainer);

        // Success
        return true;
    }

    void LOAD_RESOURCES_MK_BLIT_MINUS(BLITZEN_WORLD* pWORLD, BlitzenEngine::RenderingResources* pRenderingResources, BlitzenEngine::RenderingLoadingContextMesh& loadingContextMesh,
        int argc, char** argv)
    {

        BlitzenEngine::SCENE_CREATE_CONTEXT sceneCtx{};
        sceneCtx.pRenderer = pWORLD->BMPR.Data();
        sceneCtx.pResidents = &pWORLD->mResidents;
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

        BLIT_ASSERT(BlitGenerator::GenerateTerrainVertices(pRenderingResources->m_terrainContainer));

        //BlitzenEngine::MESH_PRIMITIVE_CREATE_CONTEXT meshPrimitiveCtx{};
        //meshPrimitiveCtx.m_indexCount = pRenderingResources->m_terrainContainer.terrainIndexCount;
        //meshPrimitiveCtx.m_indices = pRenderingResources->m_terrainContainer.terrainIndices;
        //meshPrimitiveCtx.m_materialID = 0;
        //meshPrimitiveCtx.m_vertexCount = pRenderingResources->m_terrainContainer.terrainVertexCount;
        //meshPrimitiveCtx.m_vertices = pRenderingResources->m_terrainContainer.terrainVertices;
        //pRenderingResources->m_meshContext.m_meshPrimitives.GenerateSurface(pRenderingResources->m_meshContext.m_triangles, pRenderingResources->m_meshContext.m_clusters, meshPrimitiveCtx);

        auto sceneRes{ BlitzenEngine::CreateScene(sceneCtx, loadingContextMesh) };

        BLIT_ASSERT(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)sceneRes));
        if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)sceneRes))
        {
            BLIT_ERROR("%s: Error while loading scenes. Received error message: %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, BlitzenEngine::GET_SCENE_CREATE_RES_STRING(sceneRes));
            BLIT_ASSERT(false);
        }

        BlitzenEngine::RenderingLoadingContextRenderObjects loadingContextObj{};
        if (!BlitzenEngine::UploadToColliderAMaxRadStagingBuffer_MKII(pWORLD->BMPR.Data(), loadingContextObj, pWORLD->mResidents.MColliders.MColliderAMaxRad,
            BLIT_MAX_WORLD_VARIABLE_COUNT + pWORLD->mResidents.MColliders.mStaticColliderCount))
        {
            BLIT_ERROR("%s: Failed to upload AMaxRad collider data", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            BLIT_ASSERT(false);
        }
        if (!BlitzenEngine::UploadToColliderBMinTypeStagingBuffer_MKII(pWORLD->BMPR.Data(), loadingContextObj, pWORLD->mResidents.MColliders.MColliderBMinType,
            BLIT_MAX_WORLD_VARIABLE_COUNT + pWORLD->mResidents.MColliders.mStaticColliderCount))
        {
            BLIT_ERROR("%s: Failed to upload BMinType collider data", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            BLIT_ASSERT(false);
        }

#if defined(BLIT_GAME_TEST)
        pWORLD->m_activeCameraIDX = 1;
        BlitzenWorld::SetupCameraAttachment(pWORLD->m_mainCharacter, BlitML::float3(0.f, 2.f, -4.f), BlitzenEngine::CAMERA_FREE_ROTATION_SETTING::ALWAYS);
#endif

        constexpr uint32_t CollisionGridOrigin = 0;
        pWORLD->mCollisionGrid.DefineGrid(CollisionGridOrigin);
        pWORLD->mCollisionGrid.CreateCells();
        pWORLD->mCollisionGrid.PlaceStatics(pWORLD->mResidents.mTransforms.m_transforms, pWORLD->mResidents.mTransforms.m_staticTransformCount);
        pWORLD->MBmprCollisionWorkConstant.workCount = pWORLD->mResidents.mWorldVariableCount;
        pWORLD->MBmprCollisionWorkConstant.minBounds = pWORLD->mCollisionGrid.m_minBounds;
        pWORLD->MBmprCollisionWorkConstant.maxBounds = pWORLD->mCollisionGrid.m_maxBounds;
        pWORLD->mCollisionGrid.AllocDynamicIndices();

#if defined(CUSTOM_FILE_TEST) && !defined(MOVING_RESIDENT_TEST) && !defined(DEFAULT_GLTF_SCENE_TEST) && !defined(LOAD_CMD_ARG_GLTF_FILEPATHS) && !defined(RENDERER_STRESS_TEST)

#else

        if (!BlitzenEngine::UploadResourcesToGPU(pWORLD->BMPR.Data(), pWORLD->m_drawContext, loadingContextMesh, loadingContextObj))
        {
            BLIT_FATAL("Renderer failed to setup, Blitzen shutting down");
            BLIT_ASSERT(false);
            return;
        }

#endif
        pRenderingResources->m_meshContext.m_triangles.CLEAN();
        pRenderingResources->m_meshContext.m_clusters.CLEAN();
    }

    void INITIALIZE_WORLD_POINTER(BLITZEN_WORLD* ptr)
    {
        BLIT_ASSERT_MESSAGE(GSBlitzenWorld == nullptr, "Tried to reinitialize WORLD pointer");
        GSBlitzenWorld = ptr;
    }
}