#pragma once
#include "Core/blitzenEngine.h"
#include "BlitzenMathLibrary/blitMLTypes.h"

namespace BlitzenEngine
{
    struct CameraTransformData
    {
        BlitML::mat4 rotation{};
        BlitML::mat4 translation{};
        BlitML::vec3 velocity{ 0.f };

        float yawRotation;
        float pitchRotation;
        float yawMovement;
        float pitchMovement;

        float fov;

		// Debug functionality, to freeze frustum culling
        bool bFreezeFrustum{ false };
        uint32_t debugPyramidLevel{ 0 };

        BlitML::mat4 projectionMatrix;
        BlitML::mat4 projectionTranspose;
        BlitML::mat4 onbcProjectionMatrix;
    };

    // Shader struct. Shaders are expected to have a struct that is aligned with this
    struct alignas(256) CameraViewData
    {
        BlitML::mat4 viewMatrix;
        BlitML::mat4 projectionViewMatrix;
        BlitML::vec3 position;
        float frustumRight;
        float frustumLeft;
        float frustumTop;
        float frustumBottom;
        float proj0;
        float proj5;
        float zNear;
        float zFar;
        float pyramidWidth;
        float pyramidHeight;
        float lodTarget;
        float deltaTime;
    };

    struct CameraCullData
    {
        BlitML::mat4 viewMatrix;
        float frustumRight;
        float frustumLeft;
        float frustumTop;
        float frustumBottom;
        float projection0;
        float projection5;
        float zNear;
        float zFar;
        float HI_Z_MAP_width;
        float HI_Z_MAP_height;
        float lodTarget;
        float deltaTime;
    };
    struct alignas(256) CameraCullData_HLSLCBV
    {
        BlitML::mat4 viewMatrix;
        float frustumRight;
        float frustumLeft;
        float frustumTop;
        float frustumBottom;
        float projection0;
        float projection5;
        float zNear;
        float zFar;
        float HI_Z_MAP_width;
        float HI_Z_MAP_height;
        float lodTarget;
        float deltaTime;
    };

    struct CameraClipCoordinates
    {
        BlitML::mat4 clipCoordinates;
    };

    struct CameraAttachmentSettings
    {
        uint32_t residentID = 0;
        BlitML::float3 paddingFromResident{ 0.f };
        bool residentForwardEffectFlag = true;
    };

    struct Camera
    {
        CameraViewData viewData;
        CameraTransformData transformData;
        CameraAttachmentSettings attachmentSettings;
    };

    // Default setup
    void SetupCamera(Camera& camera);

    // Updates main camera every frame
    void UpdateCamera(Camera& camera, float deltaTime);

    void UpdateResidentAttachedCamera(Camera& camera, float deltaTime);

    void CreateRotationMatrixFromPitchAndYawQuaternion(const BlitML::quat& pitchOrientation, const BlitML::quat& yawOrientation, BlitML::mat4& rotationMatrix);

    // Updates the projection matrix when necessary
    void UpdateProjection(Camera& camera, float newWidth, float newHeight);

    // Test function, taken from https://terathon.com/blog/oblique-clipping.html
    void ObliqueNearPlaneClippingMatrixModification(BlitML::mat4& proj, BlitML::mat4& res, const BlitML::vec4& clipPlane);

    void RotateResidentAttachedCamera(Camera& camera, float deltaTime);
}