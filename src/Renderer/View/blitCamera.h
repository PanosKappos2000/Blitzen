#pragma once
#include "Core/blitzenEngine.h"
#include "BlitzenMathLibrary/blitML.h"

namespace BlitzenEngine
{
    struct CameraTransformData
    {
        // Multiplied to create the view matrix
        BlitML::mat4 rotation{};
        BlitML::mat4 translation{};

        // Keeps track of the movement of the camera
        BlitML::vec3 velocity{ 0.f };

        // Keeps track of the camera's orientation
        float yawRotation;
        float pitchRotation;

        // Signifies that camera values have been updated
        bool bCameraDirty{ false };

        float windowWidth;
        float windowHeight;
        bool bWindowResize{ false };

        // Projection matrix data
        float fov;
        BlitML::mat4 projectionMatrix;
        BlitML::mat4 projectionTranspose;

		// Debug functionality, to freeze frustum culling
        bool bFreezeFrustum{ false };
        uint32_t debugPyramidLevel{ 0 };
    };

    // Shader struct. Shaders are expected to have a struct that is aligned with this
    struct alignas(256) CameraViewData
    {
        BlitML::mat4 viewMatrix;
        BlitML::mat4 projectionViewMatrix;

        BlitML::vec3 position;

        // Frustum planes
        float frustumRight;
        float frustumLeft;
        float frustumTop;
        float frustumBottom;

        // Occlusion culling projection matrix values
        float proj0;
        float proj5;

        // Used in place of frustum far and frustum near
        float zNear;
        float zFar;

        float pyramidWidth;
        float pyramidHeight;

        // Used to adapt the lod threshold to screen / view settings
        float lodTarget;
    };

    struct Camera
    {
        CameraViewData viewData;

        CameraTransformData transformData;

        BlitML::mat4 onbcProjectionMatrix;
    };

    class CameraContainer
    {
    public:

        inline CameraContainer() :m_mainCamera{ m_cameraList[BlitzenCore::Ce_MainCameraId] }
        {}

        // Returns the main camera
        inline Camera& GetMainCamera() { return m_mainCamera; }

        // Return the camera container to get access to all the available cameras
        inline Camera* GetCameras() { return m_cameraList; }

    private:

        // Holds all the camera created and an index to the active one
        Camera m_cameraList[BlitzenCore::Ce_MaxCameraCount];

        // The main camera is the one whose values are used for culling and other operations
        Camera& m_mainCamera;
    };

    void SetupCamera(Camera& camera, float fov, float windowWidth, float windowHeight, float zNear, const BlitML::vec3& initialCameraPosition, 
        float drawDistance, float initialYawRotation = 0, float initialPitchRotation = 0);

    // Default setup
    void SetupCamera(Camera& camera);

    // Updates main camera every frame
    void UpdateCamera(Camera& camera, float deltaTime);

    // Camera rotation with quats
    void RotateCamera(Camera& camera, float deltaTime, float pitchRotation, float yawRotation);

    // Updates the projection matrix when necessary
    void UpdateProjection(Camera& camera, float newWidth, float newHeight);

    // Test function, taken from https://terathon.com/blog/oblique-clipping.html
    void ObliqueNearPlaneClippingMatrixModification(BlitML::mat4& proj, BlitML::mat4& res, const BlitML::vec4& clipPlane);
}