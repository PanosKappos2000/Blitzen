#include "blitzenWorld.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/BlitzenWorld/blitzenUserInterface.h"
#include "BlitzenMathLibrary/blitMLSIMD.h"
#include "Core/WrldFileManager/blitFileManager.h"
#include "blitWorldMap.h"

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
        if constexpr (!BLITGCBroadPhaseCollisionBumper && !BLITGCBroadPhaseCollisionBumper)
        {
            BlitzenCore::BlitPerformanceCounter counter;
            counter.GenerateInner();
            double broadPhaseEnd = 0;
            double broadPhaseState = counter.Startup();
            // Broad phase. Collects the colliders inside cells. 
            // Dynamic colliders point to their cell. 
            // Each Cell points at an offset an indices array which points back to the colliders
            pWORLD->mCollisionGrid.PlaceDynamics(pWORLD->mResidents.WVTransforms, pWORLD->mResidents.mWorldVariableCount);
            broadPhaseEnd = counter.End();
            counter.Reset();
        
            // Transform collider shapes to world space. Doing this per transform for now for ease of use. 
            pWORLD->mResidents.MColliders.TransformCollidersWithoutBMPR(pWORLD->mResidents.mWorldVariableCount, pWORLD->mResidents.WVTransforms,
                pWORLD->mResidents.mTransforms.m_transforms);
        
            // Narrow phase loop. Only dynamics become hitters
            for (uint32_t id = 0; id < pWORLD->mResidents.mWithVelocityCount; ++id)
            {
                BlitzenEngine::Resident hitter = pWORLD->mResidents.WVWithVelocity[id];
                BlitzenColliderType hitterColliderType = (BlitzenColliderType)pWORLD->mResidents.MColliders.MTransformedColliderBMinType[hitter].data.w;
                auto& cell = pWORLD->mCollisionGrid.mCellOffsets[pWORLD->mResidents.WVTransforms[hitter].targetIdx];
        
                switch (hitterColliderType)
                {
                case BlitzenColliderTypeCapsule:
                    pWORLD->mResidents.MColliders.CheckCapsuleColliderInsideGridCell(hitter, cell, pWORLD->mCollisionGrid.mColliderIndices);
                    break;
                case BlitzenColliderTypeAABB:
                    pWORLD->mResidents.MColliders.CheckAABBColliderInsideGridCell(hitter, cell, pWORLD->mCollisionGrid.mColliderIndices);
                    break;
                case BlitzenColliderTypeSphere:
                    pWORLD->mResidents.MColliders.CheckSphereColliderInsideGridCell(hitter, cell, pWORLD->mCollisionGrid.mColliderIndices);
                    break;
                }
                
            }

            ResolveCollisionEvents(pWORLD->mResidents.MColliders);
        }
    }

    void ResolveCollisionEvents(BlitzenEngine::ColliderContainer& colliders)
    {
        uint32_t count = colliders.mCollisionMessageCount;
        if(colliders.mCollisionMessageCount != 0) BLIT_DBLOG("count: %u", colliders.mCollisionMessageCount);
        colliders.mCollisionMessageCount = 0;
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

#if defined(BLIT_VISUAL_DEBUG)
        if (WORLD->mDbgData.drawCollidersFlag)
        {
            BlitzenEngine::CULL_CONTEXT colliderDebugCullContext{};
            colliderDebugCullContext.m_workCount = WORLD->mResidents.m_renders.m_opaqueStaticCount + WORLD->mResidents.m_renders.m_opaqueDynamicCount;
            BlitzenEngine::BMPRDrawColliders(pRenderer, colliderDebugCullContext);
        }
#endif

        BlitzenEngine::FinalizeRendering(pRenderer);
        BlitzenEngine::EndGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::GRAPHICS);

        // Pass data back to the CPU for logic updates
        BlitzenEngine::SHADER_GAME_LOGIC_UPDATES shaderDataReadback{};
        shaderDataReadback.m_transformCount = WORLD->mResidents.mWorldVariableCount;
        shaderDataReadback.pGpuTransorms = WORLD->mResidents.WVTransforms;
        BlitzenEngine::RequestGameLogicUpdatesFromShader(pRenderer, shaderDataReadback);
        BlitzenEngine::EndGPUCommands(pRenderer, BlitzenEngine::BMPR_COMMAND_LIST_TYPE::COMPUTE);
    }

    void RegisterFrameEvent(BlitzenEngine::Resident resident, BlitzenCore::FrameEventPfn function)
    {
        GSBlitzenWorld->m_frameEvents.RegisterFrameEvent(resident, function);
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

    void SetupCameraAttachment(uint32_t residentID, BlitML::float3 paddingFromAttachment, bool residentDirectionEffectFlag)
    {
        auto& camera = GSBlitzenWorld->m_cameras[GSBlitzenWorld->m_activeCameraIDX];

        camera.attachmentSettings.residentID = residentID;
        camera.attachmentSettings.paddingFromResident = paddingFromAttachment;
        camera.attachmentSettings.residentForwardEffectFlag = residentDirectionEffectFlag;

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

    BlitML::fDirection GetResidentForward(BlitzenEngine::Resident resident)
    {
        switch (GSBlitzenWorld->mResidents.WVDirectionData[resident].directionInfluencer)
        {
        case BlitzenEngine::DirectionInfluencer::Camera:
        {
            auto& camera = GSBlitzenWorld->m_cameras[GSBlitzenWorld->m_activeCameraIDX];

            BlitML::fDirection worldDirection = 
                BlitML::ToVec3(BCPSS::MulMat4Vec4(BlitML::Mat4EulerY(camera.transformData.yawRotation), BlitML::float4(GSBlitzenWorld->mResidents.WVDirectionData[resident].intent, 0.f)));
            BlitML::Normalize(worldDirection);

            // Rotates resident to face towards direction
            if (GSBlitzenWorld->mResidents.WVTransforms[resident].movementFlags & BLIT_RESIDENT_MOVEMENT_ROTATE_TO_DIRECTION_BIT)
            {
                BlitzenEngine::SetResidentYaw(resident, BlitML::ATan2Float(worldDirection.x, worldDirection.z));
            }

            return worldDirection;
        }
        
        default:
        {
            return BlitML::fDirection(0.f);
        }
        }
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

    void AddResourceNameToWORLD(const char* resourceName)
    {
        GSBlitzenWorld->mResourceNames[GSBlitzenWorld->mResourceNameCount++].Append(const_cast<char*>(resourceName));
    }

    bool LoadWorldMapResources(BLITZEN_WORLD* pWORLD, BlitzenEngine::RenderingResources* pRenderingResources)
    {
        size_t resourceNameSizeArr[1000];
        pWORLD->mResourceNameCount = BlitzenEngine::LoadWORLDMapResourceNamesFromDisk(pWORLD->mActiveMapName, pWORLD->mResourceNames, resourceNameSizeArr);

        if(pWORLD->mResourceNameCount == BlitzenEngine::GCLoadWORLDMapResourcenamesFromDiskErrorCode)
        {
            BLIT_FATAL("%s: Failed to get resource names from map file", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        for (uint32_t n = 0; n < pWORLD->mResourceNameCount; ++n)
        {
            BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE mappedFile;
            auto loadMeshFromDiskRes = BlitzenEngine::LoadMeshFromDisk(pWORLD->mResourceNames[n].GetClassic(), mappedFile, pRenderingResources->m_meshContext);
            if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)loadMeshFromDiskRes))
            {
                BLIT_FATAL("%s: Failed to load mesh from file: %s. Error code: %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, pWORLD->mResourceNames[n].GetClassic(), 
                    BlitzenEngine::LOAD_MESH_FROM_STRING_RES_TO_STRING(loadMeshFromDiskRes));
                return false;
            }

            if (!BlitzenEngine::CopyMeshResourcesToStagingBuffer(&pRenderingResources->m_meshContext, pRenderingResources->mLoadingContextMesh))
            {
                BLIT_ERROR("%s: Failed to upload resources to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
                return false;
            }
        }

        return true;
    }

    bool RenderingResourcesInit(BLITZEN_WORLD* pWORLD, BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer)
    {
        // Calls the renderer's texture loading
        // As the default texture adds the Blitzen logo
        // This is fixed behavior for now
        pResources->m_textureManager.ALLOC();
        if (!BlitzenEngine::UploadTextureToGPU(pRenderer, pResources->m_textureManager.m_singleTextureHandle, "Assets/Textures/BlitzenLSV1.dds"))
        {
            BLIT_ERROR("%s: Rendering resources failed", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        // Configures idle work for renderer.
        // Loading is done in a separate thread.
        // The renderer can show a loading screen as it happens
        if (!BlitzenEngine::UploadRendererIdleWorkResources(pRenderer, BlitzenEngine::RENDERER_IDLE_MODE::BLITZEN_LOGO))
        {
            BLIT_ERROR("%s: Failed to put renderer on Idle Work Mode", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        // Adds the texture to the cpu side texture manager.
        // This does not mean anything for now.
        // But I will probably want to couple textures with resources and maps / grid partitions
        if (!pResources->m_textureManager.AddTexture(BlitzenCore::Ce_DefaultTextureName))
        {
            BLIT_ERROR("%s: Something went wrong with texture map", BlitzenCore::CE_WORLD_SYSTEM_NAME);
        }
        
        // Adds a default material.
        // Materials are loaded directly.
        // They do not have a binary format yet.
        if (!pResources->m_textureManager.AddMaterial(0, 0, 0, 0, BlitzenCore::Ce_DefaultMaterialName))
        {
            BLIT_ERROR("Rendering resources failed");
            return false;
        }

        // Allocates space for vertices and indices
        pResources->m_meshContext.m_triangles.ALLOC();
        pResources->m_meshContext.m_clusters.ALLOC();

        // Allocates staging buffers for mesh resources
        // When a resource is loaded the staging buffers will be invoked to hold on to it
        // Once everything in the map is ready, they will be uploaded to GPU side buffers
        BlitzenEngine::AllocateLoadingContextMesh(pRenderer, pResources->mLoadingContextMesh);

        // Gives an active map to the WORLD structure
        // For now, I only have the default map, 
        // But this is supposed to be read from the WRLD file
        GSBlitzenWorld->mActiveMapName = BlitzenEngine::GCDefaultWorldMapName;

        // This is supposed to only be defined by the build system on first load
        // Right now it does not work very well 
#if defined(CUS)

        // Creates the WRLD(project) file, for the first time.
        BLIT_ASSERT_MESSAGE(BlitzenCore::StartNewWRLDFile(), "Failed on initial project load. This is a fundamental problem with the Engine, or outside interference");
        BlitzenCore::UpdateWrldFile(GSBlitzenWorld->mActiveMapName);
        // Safety assertion for visual debug mesh resources
        BLIT_ASSERT(pResources->m_meshContext.m_meshCount == BLIT_HLSL_COLLIDER_RESOURCE_OFFSET);

        //------------------------------------------------------------------------------------------------
        // Default resources from OBJs and GLTFs are loaded for the first time below.
        // Each one is loaded to their own rpf file
        // Resource names are also added to the map files, so that the map know what it needs.
        //------------------------------------------------------------------------------------------------

        // Loads sphere mesh obj for collider debug view
        // Converts it to binary format file (.blitMesh on project folder)
        if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/sphere.obj", BlitzenEngine::GCSphereShapeMeshName))
        {
            BLIT_ERROR("%s: Failed to load sphere shape mesh resource", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }
        // Adds resource name to the world object
        GSBlitzenWorld->mResourceNames[GSBlitzenWorld->mResourceNameCount++].CopyString(const_cast<char*>(BlitzenEngine::GCSphereShapeMeshName));

        // Loads cube mesh obj for collilder debug view
        // Converts it to binary format file (.blitMesh on project folder)
        if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/cube.obj", BlitzenEngine::GCCubeShapeMeshName))
        {
            BLIT_ERROR("%s: Failed to load cube shape mesh resources", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }
        GSBlitzenWorld->mResourceNames[GSBlitzenWorld->mResourceNameCount++].CopyString(const_cast<char*>(BlitzenEngine::GCCubeShapeMeshName));

        // Loads capsule mesh obj for collider debug view
        // Converts it to binary format file (.blitMesh on project folder)
        if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/capsule.obj", BlitzenEngine::GCCapsuleShapeMeshName))
        {
            BLIT_ERROR("%s: Failed to load capsule shape mesh resources", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }
        // Adds resource name to the world object
        GSBlitzenWorld->mResourceNames[GSBlitzenWorld->mResourceNameCount++].CopyString(const_cast<char*>(BlitzenEngine::GCCapsuleShapeMeshName));

        // Loads bunny mesh obj.
        // Converts it to binary format file (.blitMesh on project folder)
        if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/bunny.obj", BlitzenEngine::GCDefaultMeshName))
        {
            BLIT_ERROR("Failed to load default bunny mesh");
            return false;
        }
        // Adds resource name to the world object
        GSBlitzenWorld->mResourceNames[GSBlitzenWorld->mResourceNameCount++].CopyString(const_cast<char*>(BlitzenEngine::GCDefaultMeshName));

        // Loads kitten mesh obj.
        // Converts it to binary format file (.blitMesh on project folder)
        if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/kitten.obj", BlitzenEngine::GCDefaultKittenMeshName))
        {
            BLIT_ERROR("%s: Failed to load default kitten mesh", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }
        // Adds resource name to the world object
        GSBlitzenWorld->mResourceNames[GSBlitzenWorld->mResourceNameCount++].CopyString(const_cast<char*>(BlitzenEngine::GCDefaultKittenMeshName));

        // Loads dragon mesh obj.
        // Converts it to binary format file (.blitMesh on project folder)
        if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/dragon.obj", BlitzenEngine::GCDefaultDragonMeshName))
        {
            BLIT_ERROR("%s: Failed to load default dragon mesh", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }
        GSBlitzenWorld->mResourceNames[GSBlitzenWorld->mResourceNameCount++].CopyString(const_cast<char*>(BlitzenEngine::GCDefaultDragonMeshName));

        // Loads dragon mesh obj.
        // Converts it to binary format file (.blitMesh on project folder)
        if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/FinalBaseMesh.obj", BlitzenEngine::GCDefaultHumanMeshName))
        {
            BLIT_ERROR("%s: Failed to load default human mesh", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }
        GSBlitzenWorld->mResourceNames[GSBlitzenWorld->mResourceNameCount++].CopyString(const_cast<char*>(BlitzenEngine::GCDefaultHumanMeshName));

        // Static access pointers
        BlitzenEngine::InitializeWorldResidentsPointer_STATIC_ACCESS(&pWORLD->mResidents);
        pWORLD->mCollisionGrid.ALLOC_IDX();
        pWORLD->mResidents.MColliders.ALLOC_MSG();
        BlitzenEngine::InitializeMeshResourcesPointer_STATIC_ACCESS(&pResources->m_meshContext);
        BlitzenEngine::InitializeTerrainContainerPtr(&pResources->m_terrainContainer);

        // All resource names are uploaded to the WORLD file
        if (!BlitzenEngine::UploadWORLDMapResourceNamesToDisk(BlitzenEngine::GCDefaultWorldMapName, GSBlitzenWorld->mResourceNames, GSBlitzenWorld->mResourceNameCount))
        {
            BLIT_ERROR("%s: Failed to load resource names to map files", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        // Terrain generation.
        // For now terrain is completely decoupled. 
        // But it will probably be a more sensible part of the pipeline in the future.
        pResources->m_terrainContainer.ALLOC();
        BLIT_ASSERT(BlitGenerator::GenerateTerrainVertices(pResources->m_terrainContainer));

        // Loads back the resources whose names were written in the map file
        if (!LoadWorldMapResources(GSBlitzenWorld, pResources))
        {
            return false;
        }

        // Popullates resident system 
        BlitzenEngine::SCENE_CREATE_CONTEXT sceneCtx{};
        sceneCtx.pRenderer = pWORLD->BMPR.Data();
        sceneCtx.pResidents = &pWORLD->mResidents;
        sceneCtx.pResources = pResources;

        BlitCL::DynamicArray<BlitzenEngine::SceneContext> scenes{};

#if defined(RENDERER_STRESS_TEST)

        // Loads the stress test
        auto geometryStressTest{ BlitzenEngine::LoadGeometryStressTest(&pWORLD->mResidents, pResources, BlitzenEngine::GCRenderingStressTestRandomTransformMultiplier) };
        BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)geometryStressTest), "Fatal error encountered while loading renderer stress test scene");
        if (BlitzenCore::BLIT_CHECK_FAIL(int64_t(geometryStressTest)))
        {
            BLIT_ERROR("%s: Failed to create rendering stress test scene. Received error: %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, BlitzenEngine::GET_SCENE_CREATE_RES_STRING(geometryStressTest));
            return false;
        }

#endif

        // Loads the moving residents
        auto movingResidentTestRes{ BlitzenEngine::LoadMovingResidentTest(&pWORLD->mResidents, BlitzenEngine::GCMovingResidentTestRandomTransformMultiplier) };
        BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)movingResidentTestRes), "Fatal error encountered while loading moving resident test scene");
        if (BlitzenCore::BLIT_CHECK_FAIL(int64_t(movingResidentTestRes)))
        {
            BLIT_ERROR("%s: Failed to create moving resident test scene. Received error: %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, BlitzenEngine::GET_SCENE_CREATE_RES_STRING(movingResidentTestRes));
            return false;
        }
        
        // Creates a grid that will be used to split residents in cells based on their position on the x and z axis
        // This position is used to lighten the load on the collision systems
        constexpr uint32_t CollisionGridOrigin = 0;
        pWORLD->mCollisionGrid.DefineGrid(CollisionGridOrigin);
        pWORLD->mCollisionGrid.CreateCells();
        pWORLD->mCollisionGrid.PlaceStatics(pWORLD->mResidents.mTransforms.m_transforms, pWORLD->mResidents.mTransforms.m_staticTransformCount);
        pWORLD->MBmprCollisionWorkConstant.workCount = pWORLD->mResidents.mWorldVariableCount;
        pWORLD->MBmprCollisionWorkConstant.minBounds = pWORLD->mCollisionGrid.m_minBounds;
        pWORLD->MBmprCollisionWorkConstant.maxBounds = pWORLD->mCollisionGrid.m_maxBounds;

        // Finally uploads to map
        auto worldMapRes = BlitzenEngine::UploadWORLDMapToDisk(GSBlitzenWorld->mActiveMapName, &pWORLD->mResidents);
        BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)worldMapRes), BlitzenEngine::GET_UPLOAD_WRLD_MAP_RES_ENUM_STRING(worldMapRes));
        if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)worldMapRes))
        {
            BLIT_ERROR("%s: Failed to upload world map to disk. Error: %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, BlitzenEngine::GET_UPLOAD_WRLD_MAP_RES_ENUM_STRING(worldMapRes));
            return false;
        }

        auto loadWorldMapRes = BlitzenEngine::LoadWORLDMapFromDisk(GSBlitzenWorld->mActiveMapName, &pWORLD->mResidents);
        BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)loadWorldMapRes), BlitzenEngine::GET_LOAD_WRLD_MAP_RES_ENUM_STRING(loadWorldMapRes));
        if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)loadWorldMapRes))
        {
            BLIT_ERROR("%s: Failed to load world map from disk. Error: %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, BlitzenEngine::GET_LOAD_WRLD_MAP_RES_ENUM_STRING(loadWorldMapRes));
            return false;
        }

#else
        GSBlitzenWorld->mActiveMapName = BlitzenEngine::GCDefaultWorldMapName;
        // Global residents pointer
        BlitzenEngine::InitializeWorldResidentsPointer_STATIC_ACCESS(&pWORLD->mResidents);
        // Grid Collider indices
        pWORLD->mCollisionGrid.ALLOC_IDX();
        // Collision Message allocation
        pWORLD->mResidents.MColliders.ALLOC_MSG();
        BlitzenEngine::InitializeMeshResourcesPointer_STATIC_ACCESS(&pResources->m_meshContext);
        BlitzenEngine::InitializeTerrainContainerPtr(&pResources->m_terrainContainer);

        pResources->m_terrainContainer.ALLOC();
        BLIT_ASSERT(BlitGenerator::GenerateTerrainVertices(pResources->m_terrainContainer));

        //Loads the resources from the map
        if (!LoadWorldMapResources(GSBlitzenWorld, pResources))
        {
            return false;
        }

        auto loadWorldMapRes = BlitzenEngine::LoadWORLDMapFromDisk(GSBlitzenWorld->mActiveMapName, &pWORLD->mResidents);
        BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)loadWorldMapRes), BlitzenEngine::GET_LOAD_WRLD_MAP_RES_ENUM_STRING(loadWorldMapRes));
        if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)loadWorldMapRes))
        {
            BLIT_ERROR("%s: Failed to load world map from disk. Error: %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, BlitzenEngine::GET_LOAD_WRLD_MAP_RES_ENUM_STRING(loadWorldMapRes));
            return false;
        }

        // Places static objects inside the grid cells after loading the resident system
        // The static placements will be loaded to the map files as well
        constexpr uint32_t CollisionGridOrigin = 0;
        pWORLD->mCollisionGrid.DefineGrid(CollisionGridOrigin);
        pWORLD->mCollisionGrid.CreateCells();
        pWORLD->mCollisionGrid.PlaceStatics(pWORLD->mResidents.mTransforms.m_transforms, pWORLD->mResidents.mTransforms.m_staticTransformCount);
        pWORLD->MBmprCollisionWorkConstant.workCount = pWORLD->mResidents.mWorldVariableCount;
        pWORLD->MBmprCollisionWorkConstant.minBounds = pWORLD->mCollisionGrid.m_minBounds;
        pWORLD->MBmprCollisionWorkConstant.maxBounds = pWORLD->mCollisionGrid.m_maxBounds;

#if defined(BLIT_VISUAL_DEBUG)

        pWORLD->mDbgData.collisionGridVertices = reinterpret_cast<BlitML::float3*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::TRIANGLE,
            sizeof(BlitML::float3) * BlitzenEngine::GCCollisionCellCount * BlitML::GCQuadVertexCount));

        pWORLD->mCollisionGrid.GenerateGridCellDrawData(pWORLD->mDbgData.collisionGridVertices);

#endif

#endif
        BlitzenEngine::RenderingLoadingContextRenderObjects loadingContextObj{};
        if constexpr (GCBlitGpuColliderFlag)
        {
            if (!BlitzenEngine::UploadToColliderAMaxRadStagingBuffer_MKII(pWORLD->BMPR.Data(), loadingContextObj, pWORLD->mResidents.MColliders.MColliderAMaxRad,
                BLIT_MAX_WORLD_VARIABLE_COUNT + pWORLD->mResidents.MColliders.mStaticColliderCount))
            {
                BLIT_ERROR("%s: Failed to upload AMaxRad collider data", BlitzenCore::CE_WORLD_SYSTEM_NAME);
                BLIT_ASSERT(false);
                return false;
            }
            if (!BlitzenEngine::UploadToColliderBMinTypeStagingBuffer_MKII(pWORLD->BMPR.Data(), loadingContextObj, pWORLD->mResidents.MColliders.MColliderBMinType,
                BLIT_MAX_WORLD_VARIABLE_COUNT + pWORLD->mResidents.MColliders.mStaticColliderCount))
            {
                BLIT_ERROR("%s: Failed to upload BMinType collider data", BlitzenCore::CE_WORLD_SYSTEM_NAME);
                BLIT_ASSERT(false);
                return false;
            }
        }

        if (!BlitzenEngine::UploadResourcesToGPU(pWORLD->BMPR.Data(), pWORLD->m_drawContext, pResources->mLoadingContextMesh, loadingContextObj))
        {
            BLIT_FATAL("Renderer failed to setup, Blitzen shutting down");
            return false;
        }

        pResources->m_meshContext.m_triangles.CLEAN();
        pResources->m_meshContext.m_clusters.CLEAN();

        // Use game like logic, instead of free flight logic
        #if defined(BLIT_GAME_TEST)
            pWORLD->m_activeCameraIDX = 1;
            BlitzenWorld::SetupCameraAttachment(pWORLD->m_mainCharacter, BlitML::float3(0.f, 2.f, -4.f), true);
        #endif

        // Success
        return true;
    }

    void LOAD_RESOURCES_MK_BLIT_MINUS(BLITZEN_WORLD* pWORLD, BlitzenEngine::RenderingResources* pRenderingResources, int argc, char** argv)
    {
        
    }

    void INITIALIZE_WORLD_POINTER(BLITZEN_WORLD* ptr)
    {
        BLIT_ASSERT_MESSAGE(GSBlitzenWorld == nullptr, "Tried to reinitialize WORLD pointer");
        GSBlitzenWorld = ptr;

        // Makes space for WORLD map resource names
        for (auto& strContainer : ptr->mResourceNames)
        {
            strContainer.Resize(BlitzenEngine::GCResourceNameMaxCount);
        }
    }

    BLITZEN_WORLD::~BLITZEN_WORLD()
    {
#if defined(BLIT_VISUAL_DEBUG)
        if (mDbgData.collisionGridVertices != nullptr)
        {
            BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::TRIANGLE, mDbgData.collisionGridVertices, sizeof(BlitML::float3) * BlitzenEngine::GCCollisionCellCount * BlitML::GCQuadVertexCount);
        }
#endif
    }
}