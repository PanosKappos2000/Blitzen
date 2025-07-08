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

    bool RenderingResourcesInit(BlitzenEngine::RenderingResources* pResources, BlitzenEngine::RendererPtrType pRenderer)
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
        uint32_t bunnyMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/bunny.obj", BlitzenCore::Ce_DefaultMeshName) };
        if (bunnyMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default bunny mesh");
            return false;
        }

        uint32_t kittenMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/kitten.obj", BlitzenCore::Ce_DefaultKittenMeshName) };
        if (kittenMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default kitten mesh");
            return false;
        }

        uint32_t dragonMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/dragon.obj", BlitzenCore::Ce_DefaultDragonMeshName) };
        if (dragonMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default dragon mesh");
            return false;
        }

        uint32_t humanMeshId{ LoadMeshFromObj(pResources->m_meshContext, "Assets/Meshes/FinalBaseMesh.obj", BlitzenCore::Ce_DefaultHumanMeshname) };
        if (humanMeshId == BlitzenCore::Ce_MaxMeshCount)
        {
            BLIT_ERROR("Failed to load default human mesh");
            return false;
        }
#endif

        BlitzenEngine::InitializeMeshResourcesPointer_STATIC_ACCESS(&pResources->m_meshContext);

        // Success
        return true;
    }

    void INITIALIZE_WORLD_POINTER(WORLD_blit* ptr)
    {
        BLIT_ASSERT_MESSAGE(p_BLITZEN_WORLD == nullptr, "Tried to reinitialize WORLD pointer");
        p_BLITZEN_WORLD = ptr;
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
}