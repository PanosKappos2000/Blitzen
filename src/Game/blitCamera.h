#pragma once
#include "Engine/blitzenEngine.h"
#include "BlitzenMathLibrary/blitML.h"

namespace BlitzenEngine
{
    // This is struct that is needed for camera movement logic and window size stats
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

		// Debug value for frustum culling, stop culling at the moment that this was turned to true,
        // effectively allowing the user to view the results of frustum culling
        // A pleasing side effect is that it also shows the effects of occlusion culling live
        bool bFreezeFrustum{ false };
    };

    // Shader struct. Shaders are expected to have a struct that is aligned with this
    struct alignas(16) CameraViewData
    {
        // The view matrix is the most important responsibility of the camera and crucial for rendering
        BlitML::mat4 viewMatrix;

        // The result of projection * view, recalculated when either of the values changes
        BlitML::mat4 projectionViewMatrix;

        // Position of the camera, used to change the translation matrix which becomes part of the view matrix
        BlitML::vec3 position;

        // Frustum data created using the transpose of the porjection matrix, 
        float frustumRight;
        float frustumLeft;
        float frustumTop;
        float frustumBottom;

        // Crucial for occlusion culling
        float proj0;// The 1st element of the projection matrix
        float proj5;// The 12th element of the projection matrix

        // zNear and zFar, important for frustum culling. zNear is also used for occlusion culling and projection matrix creation
        float zNear;
        float zFar;

        float pyramidWidth;
        float pyramidHeight;

        // Used to adapt the lod threshold to screen / view settings
        float lodTarget;
    };

    // Context for a single camera
    struct Camera
    {
        CameraViewData viewData;

        CameraTransformData transformData;

        // TODO: Fix this when I get back to my main machine, so I can properly test it
        BlitML::mat4 onbcProjectionMatrix;
    };

    // Camera setup function. Changes the values in the camera reference passed to it, using the other arguments
    void SetupCamera(Camera& camera, float fov, float windowWidth, float windowHeight, float zNear, 
        const BlitML::vec3& initialCameraPosition, float drawDistance, float initialYawRotation = 0, float initialPitchRotation = 0
    );

    // Sets up camera with engine defaults
    void SetupCamera(Camera& camera);

    // Moves a camera based on the velocity and the rotation matrix. Only works if cameraDirty is 1
    void UpdateCamera(Camera& camera, float deltaTime);

    // Rotates a camera based on the amount passed to pitchRotation and yawRotation. Does not support rollRotation for now
    void RotateCamera(Camera& camera, float deltaTime, float pitchRotation, float yawRotation);

    // Updates the projection matrix when necessary
    void UpdateProjection(Camera& camera, float newWidth, float newHeight);

    // Test function, taken from https://terathon.com/blog/oblique-clipping.html
    void ObliqueNearPlaneClippingMatrixModification(BlitML::mat4& proj, BlitML::mat4& res, const BlitML::vec4& clipPlane);

    // Camera container. Should be responsible for more things in the future
    class CameraSystem
    {
    public:
        CameraSystem();

        inline static CameraSystem* GetCameraSystem() { return m_sThis; }

        // Returns the main camera
        inline Camera& GetCamera() { return m_mainCamera; }

        // Return the camera container to get access to all the available cameras
        inline Camera* GetCameraList() { return cameraList; }

    private:

        static CameraSystem* m_sThis;

        // Holds all the camera created and an index to the active one
        Camera cameraList[MaxCameraCount];

        // The main camera is the one whose values are used for culling and other operations
        Camera& m_mainCamera;
    };
}