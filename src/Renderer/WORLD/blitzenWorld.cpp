#include "blitzenWorld.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/BlitzenWorld/blitzenUserInterface.h"
#include "BlitzenMathLibrary/blitMLSIMD.h"
#include "Core/WrldFileManager/blitFileManager.h"
#include "blitWorldMap.h"
#include "Renderer/Scene/gltfScene.h"
//#include BLITZEN_CLIENT_PATH_TO_WRLD_MAIN

namespace BlitzenWorld
{
    inline BLITZEN_WORLD* GSBlitzenWorld = nullptr;

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

    bool SwitchWorldMapFile(const char* mapName, BLITZEN_WORLD* pWORLD, BlitzenEngine::RenderingResources* pResources)
    {
        pWORLD->mActiveMapName = mapName;

        if (!BlitzenEngine::CreateWorldMapDirectory(mapName))
        {
            return false;
        }

        if (!BlitzenEngine::OpenResourceNamesBMSTRFile(pWORLD->mActiveMapName, pWORLD->mResourcesNamesFile))
        {
            BLIT_ERROR("%s: Failed to open bmstr file with resource names for map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
            return false;
        }

        if (!BlitzenEngine::OpenBINSTRFileForTextureNameWriting(pResources->mWorldMapTextureNamesBINSTRFileHandle, pWORLD->mActiveMapName))
        {
            BLIT_ERROR("%s: Failed to open Binstr file for texture name writing \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
            return false;
        }

        if (!BlitzenEngine::OpenMaterialBatchBMSTRFile(pWORLD->mActiveMapName, pResources->mMaterialBatchNamesFile))
        {
            BLIT_ERROR("%s: Failed to open bmstr file for material batch names for world map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
            return false;
        }

        if (!BlitzenEngine::OpenWorldMapResourcesContextFileForWriting(pWORLD->mWorldMapResourceContextFile, pWORLD->mActiveMapName))
        {
            BLIT_ERROR("%s: Failed to open resource context binary file for world map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
            return false;
        }

        return true;
    }

    bool LoadWorldMapResources(BLITZEN_WORLD* pWORLD, BlitzenEngine::RenderingResources* pRenderingResources)
    {
        pRenderingResources->CloseWorldMapTextureNamesBINSTRFile();

        if (!LoadWorldMapTexturesToGPU(pWORLD, pRenderingResources))
        {
            BLIT_ERROR("%s: Failed to load textures for map \"%s\" to GPU", BlitzenCore::CE_WORLD_SYSTEM_NAME, pWORLD->mActiveMapName);
            return false;
        }

        BlitzenCore::BLIT_PTR materialTextureOffsetsBuffer;
        BlitzenCore::BLIT_PTR geometryMaterialOffsetsBuffer;
        if (!BlitzenEngine::LoadWorldMapResourcesContextFromDisk(pWORLD->mActiveMapName, materialTextureOffsetsBuffer, geometryMaterialOffsetsBuffer))
        {
            BLIT_FATAL("%s: WORLD MAP RESOURCE LOADING FAILED", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }
        uint32_t* materialTextureOffsets = reinterpret_cast<uint32_t*>(materialTextureOffsetsBuffer.mPtr);
        uint32_t* geometryMaterialOffsets = reinterpret_cast<uint32_t*>(geometryMaterialOffsetsBuffer.mPtr);

        //-----------------------------------------------------------------------------------------------------------------------------------------------
        //                                                  MATERIAL LOADING
        //----------------------------------------------------------------------------------------------------------------------------------------------
        BlitzenPlatform::C_FILE_SCOPE materialNamesFile;
        if (!BlitzenEngine::OpenMaterialBatchNamesBmstrFileForRead(materialNamesFile, pWORLD->mActiveMapName))
        {
            BLIT_ERROR("%s: Failed to open BMSTR file for material batch names for world map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, pWORLD->mActiveMapName);
            return false;
        }
        
        BlitzenCore::BLIT_PTR materialBatchNameBuffer{};
        materialBatchNameBuffer.Init(1000);
        char* materialNames = reinterpret_cast<char*>(materialBatchNameBuffer.mPtr);
        uint32_t materialBatchIteration = 0;
        while (true)
        {
            auto loadStringRes = BlitzenEngine::ReadBmstrFileNextLine(materialNamesFile, &materialNames);
            if (loadStringRes == BlitzenEngine::BMSTRFileReadRes::End) break;
            if (loadStringRes == BlitzenEngine::BMSTRFileReadRes::Error) return false;
        
            BlitzenCore::BLIT_PTR materialIndicesBuffer;
            BlitzenCore::BLIT_PTR materialDataBuffer;
            uint32_t materialCountReadback;
            if (!BlitzenEngine::LoadMaterialsFromDisk(materialNames, materialIndicesBuffer, materialDataBuffer, materialCountReadback))
            {
                BLIT_ERROR("%s: Failed to load material batch no %u for map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, materialBatchIteration, pWORLD->mActiveMapName);
                return false;
            }
        
            auto materialArr = reinterpret_cast<BlitzenEngine::Material*>(materialIndicesBuffer.mPtr);
            auto materialDataArr = reinterpret_cast<BlitzenEngine::MaterialData*>(materialDataBuffer.mPtr);

            uint32_t materialTextureOffset = materialTextureOffsets[materialBatchIteration];
        
            for (uint32_t m = 0; m < materialCountReadback; ++m)
            {
                auto& material = materialArr[m];
        
                material.albedoTag = material.albedoTag == UINT32_MAX ? BLIT_BLANK_MATERIAL_INDEX : material.albedoTag + materialTextureOffset;
                material.normalTag = material.normalTag == UINT32_MAX ? BLIT_BLANK_MATERIAL_INDEX : material.normalTag + materialTextureOffset;
                material.emissiveTag = material.emissiveTag == UINT32_MAX ? BLIT_BLANK_MATERIAL_INDEX : material.emissiveTag + materialTextureOffset;
                material.specularTag = material.specularTag == UINT32_MAX ? BLIT_BLANK_MATERIAL_INDEX : material.specularTag + materialTextureOffset;

                pRenderingResources->mMaterials.mDataCount++;
            }
        
            if (!BlitzenEngine::UploadMaterialsToStagingBuffer(pRenderingResources->mLoadingContextMaterial, materialArr, materialCountReadback))
            {
                BLIT_ERROR("%s: Failed to write materials to staging buffer from batch no %s for map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, pWORLD->mActiveMapName);
                return false;
            }
        
            materialBatchIteration++;
        }

        BlitCL::String buffer{ BlitzenEngine::GCResourceNameMaxSize };
        BlitzenPlatform::C_FILE_SCOPE bmstrFile;
        if (!BlitzenEngine::OpenWorldMapBmstrFileForResourceNameReadback(bmstrFile, pWORLD->mActiveMapName))
        {
            return false;
        }
        
        while (true)
        {
            auto loadNameRes = BlitzenEngine::ReadBmstrFileNextLine(bmstrFile, buffer.GetDataPointer());
            if (loadNameRes == BlitzenEngine::BMSTRFileReadRes::End) break;
            if (loadNameRes == BlitzenEngine::BMSTRFileReadRes::Error) return false;

            BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE mappedFile;
            auto loadMeshFromDiskRes = BlitzenEngine::LoadMeshFromDisk(buffer.GetClassic(), mappedFile, pRenderingResources->m_meshContext);
            if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)loadMeshFromDiskRes))
            {
                BLIT_FATAL("%s: Failed to load mesh from file: %s. Error code: %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, buffer.GetClassic(),
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

    bool AddSingleTextureToWorldMap(const char* name, const char* textureName, BLITZEN_WORLD* pWORLD, BlitzenEngine::RenderingResources* pRenderingResources)
    {
        // Copies all texture names one after another to the name pool
        size_t stringSize = strlen(BlitzenEngine::GCRpfTextureSubfolder) + strlen("/") + strlen(textureName) + strlen(".dds");
        BlitzenCore::BLIT_PTR textureNameMem;
        textureNameMem.Init(stringSize + 1);
        char* namePtr = reinterpret_cast<char*>(textureNameMem.mPtr);
        snprintf(namePtr, stringSize + 1, "%s/%s.dds", BlitzenEngine::GCRpfTextureSubfolder, textureName);
        size_t sizeData[1] = { stringSize };
        constexpr uint32_t LCSingleStringCount = 1;

        if (!BlitzenEngine::UploadStringDataToBINSTRFile(pRenderingResources->mWorldMapTextureNamesBINSTRFileHandle, namePtr, sizeData, 
            LCSingleStringCount, (uint32_t)sizeData[0], BlitzenEngine::GCMaxLoadedTextureCount * sizeof(size_t), 
            size_t(BlitzenEngine::GCMaxLoadedTextureCount * BlitzenEngine::GCWorldMapTextureNameMaxSize)))
        {
            BLIT_ERROR("%s: Failed to write single texture name \"%s\"to binstr file", BlitzenCore::CE_WORLD_SYSTEM_NAME, textureName);
            return false;
        }

        return true;
    }

    bool LoadWorldMapTexturesToGPU(BLITZEN_WORLD* pWORLD, BlitzenEngine::RenderingResources* pRenderingResources)
    {
        size_t resourceDirectoryPathStringLength = strlen(BlitzenEngine::GCRapidMeshDirectoryPath);

        BlitCL::FatString filepath{ strlen(BlitzenEngine::GCClientWorldMapDirectory) + strlen(pWORLD->mActiveMapName) + strlen("/") + strlen(BlitzenEngine::GCNameOfWorldMapTextureNamesBINSTRFile) };
        filepath.Format("%s%s/%s", BlitzenEngine::GCClientWorldMapDirectory, pWORLD->mActiveMapName, BlitzenEngine::GCNameOfWorldMapTextureNamesBINSTRFile);
        BlitzenCore::BLIT_PTR texturesNameDataReadback;
        BlitzenCore::BLIT_PTR texturesSizeDataReadback;
        uint32_t stringCountReadback;
        if (!BlitzenEngine::LoadStringDataFromBINSTRFile(filepath.Get(), texturesNameDataReadback, texturesSizeDataReadback, stringCountReadback, 
            BlitzenEngine::GCMaxLoadedTextureCount * sizeof(size_t), BlitzenEngine::GCMaxLoadedTextureCount * BlitzenEngine::GCWorldMapTextureNameMaxSize))
        {
            BLIT_ERROR("%s: Failed to read texture names", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        BlitzenCore::BLIT_PTR textureNameContainer;
        textureNameContainer.Init(resourceDirectoryPathStringLength + BlitzenEngine::GCWorldMapTextureNameMaxSize);

        char* texNameHandle = reinterpret_cast<char*>(textureNameContainer.mPtr);
        size_t* texSizesArr = reinterpret_cast<size_t*>(texturesSizeDataReadback.mPtr);
        char* texNamesBuffer = reinterpret_cast<char*>(texturesNameDataReadback.mPtr);

        for (uint32_t t = pRenderingResources->m_textureManager.mTextureCount; t < stringCountReadback; ++t)
        {
            snprintf(texNameHandle, resourceDirectoryPathStringLength + texSizesArr[t] + 1, "%s%s", BlitzenEngine::GCRapidMeshDirectoryPath,
                &texNamesBuffer[pRenderingResources->m_textureManager.mTextureNamesBufferSize]);
            pRenderingResources->m_textureManager.mTextureNamesBufferSize += texSizesArr[t];

            if (!BlitzenEngine::UploadTextureToGPU(pWORLD->BMPR.Data(), pRenderingResources->mLoadingContextMaterial, texNameHandle))
            {
                BLIT_ERROR("%s: Failed to upload texture data to GPU buffers", BlitzenCore::CE_WORLD_SYSTEM_NAME);
                return false;
            }
        }

        pRenderingResources->m_textureManager.mTextureCount += stringCountReadback;

        return true;
    }

    bool AddSceneToWORLDMap(const char* sceneName, BLITZEN_WORLD* pWORLD, BlitzenEngine::RenderingResources* pRenderingResources)
    {
        uint32_t previousResourceCount = pRenderingResources->m_meshContext.m_meshPrimitives.m_meshPrimitivesCount;
        uint32_t previousTextureCount = pRenderingResources->m_textureManager.mTextureCount;
        uint32_t previousMaterialCount = pRenderingResources->mMaterials.mDataCount;
        size_t sceneNameLength = strlen(sceneName);
        size_t sceneTexturePrefixNameLength = strlen("texture");
        size_t ddsExtensionNameLength = strlen(".dds");
        size_t resourceDirectoryPathStringLength = strlen(BlitzenEngine::GCRapidMeshDirectoryPath);
        constexpr size_t LCDirectorySeparatorCharLength = 1;
        constexpr size_t LCNullTerminatorLength = 1;

        BLIT_ASSERT(previousTextureCount != BLIT_BLANK_MATERIAL_INDEX);

        // Starts off by retrieving the static node data which is loaded to the disk in scene context
        // This also saves the resource count for the scene and the amount of nodes / residents
        // The node data is placed directly to the resident arrays in the correct offset
        uint32_t resourceCount;
        uint32_t nodesCount;
        uint32_t texturesCount;
        BlitzenCore::BLIT_PTR renderObjects;
        BlitzenCore::BLIT_PTR meshTransforms;
        if (!BlitzenEngine::LoadImportedSceneNodesFromDisk(sceneName, resourceCount, nodesCount, texturesCount,
            BlitzenCore::Ce_MaxMeshPrimitivesCount - previousResourceCount,
            BLIT_MAX_WORLD_OPAQUE_STATIC_RENDERS - pWORLD->mResidents.m_renders.m_opaqueStaticCount,
            BlitzenEngine::GCMaxLoadedTextureCount - pRenderingResources->m_textureManager.mTextureCount,
            renderObjects, meshTransforms))
        {
            BLIT_ERROR("%s: Failed to retrieve scene nodes from disk", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        //-----------------------------------------------------------------------------------------------------------------------------------------------
        //                                                  TEXTURE LOADING
        //----------------------------------------------------------------------------------------------------------------------------------------------
        // Texture names for scene are saved in a binary string file
        size_t textureNamePoolSize = 0;
        BlitCL::DynamicArray<size_t> textureNameSizes{ texturesCount };
        for (uint32_t t = 0; t < texturesCount; ++t)
        {
            auto& nameSize = textureNameSizes[t];
            uint32_t textureNumDigits = 1;
            uint32_t textureNum = t;
            // NOTE TO SELF: Could be small function in BlitML
            while (textureNum / 10 != 0)
            {
                textureNumDigits++;
                textureNum /= 10;
            }
            nameSize = sceneNameLength + LCDirectorySeparatorCharLength + sceneTexturePrefixNameLength + textureNumDigits + ddsExtensionNameLength;
            textureNamePoolSize += nameSize;
        }

        textureNamePoolSize += LCNullTerminatorLength;

        // Copies all texture names one after another to the name pool
        BlitzenCore::BLIT_PTR texturesNamePool;
        size_t texturePoolOffset = 0;
        texturesNamePool.Init(textureNamePoolSize);
        for (uint32_t t = 0; t < texturesCount; ++t)
        {
            char* poolPtr = &reinterpret_cast<char*>(texturesNamePool.mPtr)[texturePoolOffset];
            snprintf(poolPtr, textureNameSizes[t] + 1, "%s/texture%u.dds", sceneName, t);
            texturePoolOffset += textureNameSizes[t];
        }

        if (!BlitzenEngine::OpenBINSTRFileForTextureNameWriting(pRenderingResources->mWorldMapTextureNamesBINSTRFileHandle, pWORLD->mActiveMapName))
        {
            BLIT_ERROR("%s: Failed to open Binstr file for texture name writing", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        if (!BlitzenEngine::UploadStringDataToBINSTRFile(pRenderingResources->mWorldMapTextureNamesBINSTRFileHandle, reinterpret_cast<char*>(texturesNamePool.mPtr), textureNameSizes.Data(), 
            texturesCount, (uint32_t)textureNamePoolSize, BlitzenEngine::GCMaxLoadedTextureCount * sizeof(size_t), 
            size_t(BlitzenEngine::GCMaxLoadedTextureCount * BlitzenEngine::GCWorldMapTextureNameMaxSize)))
        {
            BLIT_ERROR("%s: Failed to retrieve texture names for scene \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, sceneName);
            return false;
        }

        pRenderingResources->CloseWorldMapTextureNamesBINSTRFile();

        if (!LoadWorldMapTexturesToGPU(pWORLD, pRenderingResources))
        {
            BLIT_ERROR("%s: Failed to load scene \"%s\" texture data to GPU", BlitzenCore::CE_WORLD_SYSTEM_NAME, sceneName);
            return false;
        }


        //-----------------------------------------------------------------------------------------------------------------------------------------------
        //                                                  MATERIAL LOADING
        //----------------------------------------------------------------------------------------------------------------------------------------------
        BlitCL::FatString filepath{ strlen(BlitzenEngine::GCRapidMeshDirectoryPath) + strlen(sceneName) + strlen("/") + strlen("matBatch.blitMat") };
        filepath.Format("%s%s/matBatch.blitMat", BlitzenEngine::GCRapidMeshDirectoryPath, sceneName);

        // Uploads the name of the material batch file that was loaded
        // This allows the map to realod the material
        if (!BlitzenEngine::UploadMaterialBatchNameToDisk(pRenderingResources->mMaterialBatchNamesFile, filepath.Get()))
        {
            BLIT_ERROR("%s: Failed to write material names for scene %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, sceneName);
            return false;
        }

        BlitzenCore::BLIT_PTR materialIndicesBuffer;
        BlitzenCore::BLIT_PTR materialDataBuffer;
        uint32_t materialCountReadback;
        if (!BlitzenEngine::LoadMaterialsFromDisk(filepath.Get(), materialIndicesBuffer, materialDataBuffer, materialCountReadback))
        {
            BLIT_ERROR("%s: Failed to load materials for scene %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, sceneName);
            return false;
        }

        auto materialArr = reinterpret_cast<BlitzenEngine::Material*>(materialIndicesBuffer.mPtr);
        auto materialDataArr = reinterpret_cast<BlitzenEngine::MaterialData*>(materialDataBuffer.mPtr);

        // Adapts material texture indices to current map context
        for (uint32_t m = 0; m < materialCountReadback; ++m)
        {
            auto& material = materialArr[m];

            material.albedoTag = material.albedoTag == UINT32_MAX ? BLIT_BLANK_MATERIAL_INDEX : material.albedoTag + previousTextureCount;
            material.normalTag = material.normalTag == UINT32_MAX ? BLIT_BLANK_MATERIAL_INDEX : material.normalTag + previousTextureCount;
            material.emissiveTag = material.emissiveTag == UINT32_MAX ? BLIT_BLANK_MATERIAL_INDEX : material.emissiveTag + previousTextureCount;
            material.specularTag = material.specularTag == UINT32_MAX ? BLIT_BLANK_MATERIAL_INDEX : material.specularTag + previousTextureCount;

            //BLIT_ASSERT_MESSAGE(pRenderingResources->mMaterials.AddMaterialData(materialDataArr[m].transparencyFlag), "Gltf scene added too many material to map context");
            pRenderingResources->mMaterials.mDataCount++;
        }

        pRenderingResources->m_textureManager.mTextureCount += texturesCount;

        if (!BlitzenEngine::UploadMaterialsToStagingBuffer(pRenderingResources->mLoadingContextMaterial, materialArr, materialCountReadback))
        {
            BLIT_ERROR("%s: Failed to write materials to staging buffer for scene \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, sceneName);
            return false;
        }
        
        // With the resource count retrieved, it can now use it to find all scene resource names
        // It writes each one to the maps bmString file for resouce names
        BlitCL::FatString stringContainer{ strlen(sceneName) + strlen("/") + strlen("mesh") + 16 };
        for (uint32_t n = 0; n < resourceCount; n++)
        {
            stringContainer.Format("%s/mesh%u", sceneName, n);

            if (!BlitzenPlatform::FilesystemWriteLine(pWORLD->mResourcesNamesFile, stringContainer.Get()))
            {
                BLIT_ERROR("%s: Failed to write resource name %u from scene %s to world map resource .bmstr file", BlitzenCore::CE_WORLD_SYSTEM_NAME, n, sceneName);
                return false;
            }

            BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE mappedFile;
            auto loadMeshFromDiskRes = BlitzenEngine::LoadMeshFromDisk(stringContainer.Get(), mappedFile, pRenderingResources->m_meshContext);
            if (BlitzenCore::BLIT_CHECK_FAIL((int64_t)loadMeshFromDiskRes))
            {
                BLIT_FATAL("%s: Failed to load mesh from file: %s. Error code: %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, stringContainer.Get(),
                    BlitzenEngine::LOAD_MESH_FROM_STRING_RES_TO_STRING(loadMeshFromDiskRes));
                return false;
            }

            if (!BlitzenEngine::CopyMeshResourcesToStagingBuffer(&pRenderingResources->m_meshContext, pRenderingResources->mLoadingContextMesh))
            {
                BLIT_ERROR("%s: Failed to upload resources to staging buffer", BlitzenCore::CE_WORLD_SYSTEM_NAME);
                return false;
            }
        }

        auto renderArr = reinterpret_cast<BlitzenEngine::RenderObject*>(renderObjects.mPtr);
        auto transformArr = reinterpret_cast<BlitzenEngine::MeshTransform*>(meshTransforms.mPtr);
        //// Nodes are then adjusted to residents one by one
        for (uint32_t n = 0; n < nodesCount; ++n)
        {
            auto& render = renderArr[n];
        
            render.transformId += pWORLD->mResidents.mTransforms.m_staticTransformCount + BLIT_MAX_WORLD_VARIABLE_COUNT;
            render.surfaceId += previousResourceCount;

            BlitzenEngine::RESIDENT_CREATE_CONTEXT residentContext;
            residentContext.m_isMoveable = BLIT_FAT_FALSE;
            residentContext.m_resourceID = render.surfaceId;
            residentContext.m_transformInfo.m_pTransform = &transformArr[n];
            residentContext.snapDownOffset = 1.f;// NOTE TO SELF: This is temporary, to avoid putting the snap down effect on GLTFs
            auto residentRes = pWORLD->mResidents.AddResident(residentContext);
        }

        uint32_t previousMaterialTextureOffsetsCount = pRenderingResources->mMaterials.mOffsetCount;
        BLIT_ASSERT_MESSAGE(pRenderingResources->mMaterials.AddMaterialTextureOffsets(previousTextureCount), "Gltf scene added too many materials to map context");

        uint32_t geometryMaterialOffsetsPlaceholder = 0;
        BLIT_ASSERT_MESSAGE(BlitzenEngine::UploadWorldMapResourceContextToDisk(pWORLD->mWorldMapResourceContextFile, 
            &pRenderingResources->mMaterials.mMaterialTextureOffsets[previousMaterialTextureOffsetsCount], 1, &geometryMaterialOffsetsPlaceholder, 1), 
            "Failed to upload material texture offsets to disk for scene material batch");

        return true;
    }

    bool RenderingResourcesInit(BLITZEN_WORLD* pWORLD, BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer)
    {
        auto wrldRes = BlitzenCore::LoadWrld();
        BLIT_ASSERT(wrldRes != BlitzenCore::WrldLoadRes::BLITZEN_CLIENT_FAILED);
        if(wrldRes == BlitzenCore::WrldLoadRes::START_NEW) BlitzenCore::UpdateWrldFile(GSBlitzenWorld->mActiveMapName);

        if (!BlitzenEngine::AllocateLoadingStagingBufferMaterials(pWORLD->BMPR.Data(), pResources->mLoadingContextMaterial))
        {
            BLIT_ERROR("%s: Failed to allocate staging buffer for textures", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        if (!BlitzenEngine::AllocateLoadingContextMesh(pRenderer, pResources->mLoadingContextMesh))
        {
            BLIT_ERROR("%s: Failed to allocated loading context mesh staging buffers", BlitzenCore::CE_WORLD_SYSTEM_NAME);
            return false;
        }

        // Allocates space for vertices and indices
        pResources->m_meshContext.m_triangles.ALLOC();
        pResources->m_meshContext.m_clusters.ALLOC();

        if (!SwitchWorldMapFile(BlitzenEngine::GCDefaultWorldMapName, pWORLD, pResources))
        {
            return false;
        }

        pResources->mMaterials.ALLOC(1);
        
        if (wrldRes == BlitzenCore::WrldLoadRes::START_NEW)
        {
            if (!pResources->m_textureManager.AddTexture("BlitzenLogo.dds", "Assets/Textures/BlitzenLSV1.dds"))
            {
                BLIT_ERROR("%s: Failed to add Blitzen Logo texture", BlitzenCore::CE_WORLD_SYSTEM_NAME);
                return false;
            }

            if (!AddSingleTextureToWorldMap(pWORLD->mActiveMapName, "BlitzenLogo", pWORLD, pResources))
            {
                BLIT_ERROR("%s: Failed to add Blitzen logo texture to world map %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, pWORLD->mActiveMapName);
                return false;
            }

            //------------------------------------------------------------------------------------------------
            // Default resources from OBJs and GLTFs are loaded for the first time below.
            // Each one is loaded to their own rpf file
            // Resource names are also added to the map files, so that the map know what it needs.
            //------------------------------------------------------------------------------------------------
            if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/sphere.obj", BlitzenEngine::GCSphereShapeMeshName))
            {
                BLIT_ERROR("%s: Failed to load sphere shape mesh resource", BlitzenCore::CE_WORLD_SYSTEM_NAME);
                return false;
            }
            if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/cube.obj", BlitzenEngine::GCCubeShapeMeshName))
            {
                BLIT_ERROR("%s: Failed to load cube shape mesh resources", BlitzenCore::CE_WORLD_SYSTEM_NAME);
                return false;
            }
            if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/capsule.obj", BlitzenEngine::GCCapsuleShapeMeshName))
            {
                BLIT_ERROR("%s: Failed to load capsule shape mesh resources", BlitzenCore::CE_WORLD_SYSTEM_NAME);
                return false;
            }
            if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/bunny.obj", BlitzenEngine::GCDefaultMeshName))
            {
                BLIT_ERROR("Failed to load default bunny mesh");
                return false;
            }
            if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/kitten.obj", BlitzenEngine::GCDefaultKittenMeshName))
            {
                BLIT_ERROR("%s: Failed to load default kitten mesh", BlitzenCore::CE_WORLD_SYSTEM_NAME);
                return false;
            }
            if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/dragon.obj", BlitzenEngine::GCDefaultDragonMeshName))
            {
                BLIT_ERROR("%s: Failed to load default dragon mesh", BlitzenCore::CE_WORLD_SYSTEM_NAME);
                return false;
            }
            if (!BlitzenEngine::LoadObjFileMeshToDisk(pResources->m_meshContext, "Assets/Meshes/FinalBaseMesh.obj", BlitzenEngine::GCDefaultHumanMeshName))
            {
                BLIT_ERROR("%s: Failed to load default human mesh", BlitzenCore::CE_WORLD_SYSTEM_NAME);
                return false;
            }

            //-----------------------------------------------------------------------------------------------------------------
            // Uploads resource names to bmstr file that will be used for the map to remember the resources it needs.
            //-----------------------------------------------------------------------------------------------------------------
            if (!BlitzenEngine::UploadWORLDMapResourceNameToDisk(BlitzenEngine::GCSphereShapeMeshName, pWORLD->mResourcesNamesFile))
            {
                BLIT_ERROR("%s: Failed to write sphere shape mesh resource name to bmstr file");
                return false;
            }
            if (!BlitzenEngine::UploadWORLDMapResourceNameToDisk(BlitzenEngine::GCCubeShapeMeshName, pWORLD->mResourcesNamesFile))
            {
                BLIT_ERROR("%s: Failed to write cube shape mesh resource name to bmstr file");
                return false;
            }
            if (!BlitzenEngine::UploadWORLDMapResourceNameToDisk(BlitzenEngine::GCCapsuleShapeMeshName, pWORLD->mResourcesNamesFile))
            {
                BLIT_ERROR("%s: Failed to write capsule shape mesh resource name to bmstr file");
                return false;
            }
            if (!BlitzenEngine::UploadWORLDMapResourceNameToDisk(BlitzenEngine::GCDefaultMeshName, pWORLD->mResourcesNamesFile))
            {
                BLIT_ERROR("%s: Failed to write bunny mesh resource name to bmstr file");
                return false;
            }
            if (!BlitzenEngine::UploadWORLDMapResourceNameToDisk(BlitzenEngine::GCDefaultKittenMeshName, pWORLD->mResourcesNamesFile))
            {
                BLIT_ERROR("%s: Failed to write kitten mesh resource name to bmstr file");
                return false;
            }
            if (!BlitzenEngine::UploadWORLDMapResourceNameToDisk(BlitzenEngine::GCDefaultDragonMeshName, pWORLD->mResourcesNamesFile))
            {
                BLIT_ERROR("%s: Failed to write dragon mesh resource name to bmstr file");
                return false;
            }
            if (!BlitzenEngine::UploadWORLDMapResourceNameToDisk(BlitzenEngine::GCDefaultHumanMeshName, pWORLD->mResourcesNamesFile))
            {
                BLIT_ERROR("%s: Failed to write human mesh resource name to bmstr file");
                return false;
            }

            // Static access pointers
            BlitzenEngine::InitializeWorldResidentsPointer_STATIC_ACCESS(&pWORLD->mResidents);
            pWORLD->mCollisionGrid.ALLOC_IDX();
            pWORLD->mResidents.MColliders.ALLOC_MSG();
            BlitzenEngine::InitializeMeshResourcesPointer_STATIC_ACCESS(&pResources->m_meshContext);
            BlitzenEngine::InitializeTerrainContainerPtr(&pResources->m_terrainContainer);

#if defined(GLTF_IMPORT_TEST)
            // Imports sponza scene from gltf file
            // Adds the sponza scene name to the map
            auto gltfSceneTestRes{ BlitzenEngine::ManageGltf("C:/Dev/GltfTestScenes/Scenes/Sponza/scene.gltf", "sponza", pResources, &pWORLD->mResidents, pRenderer) };
            BLIT_ASSERT_MESSAGE(!BlitzenCore::BLIT_CHECK_FATAL((int64_t)gltfSceneTestRes), "Fatal error encountered while loading gltf scene");
            if (BlitzenCore::BLIT_CHECK_FAIL(int64_t(gltfSceneTestRes)))
            {
                BLIT_ERROR("%s: Failed to create gltf scene", BlitzenCore::CE_SCENE_SYSTEM_NAME);
                return false;
            }
#endif

            // Terrain generation.
            // For now terrain is completely decoupled. 
            // But it will probably be a more sensible part of the pipeline in the future.
            pResources->m_terrainContainer.ALLOC();
            BLIT_ASSERT(BlitGenerator::GenerateTerrainVertices(pResources->m_terrainContainer));

            pWORLD->mWorldMapResourceContextFile.Close();
            pResources->CloseWorldMapMaterialBatchNameBMSTRFile();
            // Loads back the resources whose names were written in the map file
            if (!LoadWorldMapResources(pWORLD, pResources))
            {
                return false;
            }
            // NOTE TO SELF: This is a workaround. Normally, loadWorldMapResources should NOT be called here. This is the final load function.
            // At the point that we are here, WE ARE SUPPOSED TO BE IN DEV MODE AND ACCEPTING NEW RESOURCES AND RESIDENTS TO THE WORLD
            if (!BlitzenEngine::OpenMaterialBatchBMSTRFile(pWORLD->mActiveMapName, pResources->mMaterialBatchNamesFile))
            {
                BLIT_ERROR("%s: Failed to open bmstr file for material batch names for world map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, pWORLD->mActiveMapName);
                return false;
            }
            if (!BlitzenEngine::OpenWorldMapResourcesContextFileForWriting(pWORLD->mWorldMapResourceContextFile, pWORLD->mActiveMapName))
            {
                BLIT_ERROR("%s: Failed to open resource context binary file for world map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, pWORLD->mActiveMapName);
                return false;
            }

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

#if defined(GLTF_IMPORT_TEST)
            if (!AddSceneToWORLDMap("sponza", pWORLD, pResources))
            {
                return false;
            }
#endif

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
        }
        else
        {
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
            pWORLD->mWorldMapResourceContextFile.Close();
            pResources->CloseWorldMapMaterialBatchNameBMSTRFile();
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

        }
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

        if (!BlitzenEngine::UploadResourcesToGPU(pWORLD->BMPR.Data(), pWORLD->m_drawContext, pResources->mLoadingContextMesh, loadingContextObj, pResources->mLoadingContextMaterial))
        {
            BLIT_FATAL("Renderer failed to setup, Blitzen shutting down");
            return false;
        }

        // NOTE TO SELF: This has become useless. The loading for load screens should be its own thing that is done at the start
        if (!BlitzenEngine::UploadRendererIdleWorkResources(pRenderer, BlitzenEngine::RENDERER_IDLE_MODE::BLITZEN_LOGO))
        {
            BLIT_ERROR("%s: Failed to put renderer on Idle Work Mode", BlitzenCore::CE_WORLD_SYSTEM_NAME);
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

    static void RotatingKittenFunc(BlitzenEngine::Resident resident, float deltaTime)
    {
        constexpr float movementSpeed = 1.f;
        BlitzenEngine::RotateResidentYaw(resident, movementSpeed, deltaTime);
        BlitzenEngine::RotateResidentPitch(resident, movementSpeed, deltaTime);
    }

    void LOAD_RESOURCES_MK_BLIT_MINUS(BLITZEN_WORLD* pWORLD, BlitzenEngine::RenderingResources* pRenderingResources, int argc, char** argv)
    {
        //WrldStart();
        //BlitzenEngine::RegisterFrameEventForWorldVariableType(BlitzenEngine::GCRotatingKittenWVID, RotatingKittenFunc);
    }

    void INITIALIZE_WORLD_POINTER(BLITZEN_WORLD* ptr)
    {
        BLIT_ASSERT_MESSAGE(GSBlitzenWorld == nullptr, "Tried to reinitialize WORLD pointer");
        GSBlitzenWorld = ptr;
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