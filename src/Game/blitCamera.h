#pragma once

// The camera will need to access math library types and functions
#include "BlitzenMathLibrary/blitML.h"

// The engine will hold an array
#define BLIT_MAX_CAMERAS_IN_SCENE    2
#define BLIT_MAIN_CAMERA_ID          0
#define BLIT_DETATCHED_CAMERA_ID     1

namespace BlitzenEngine
{
    // This is struct that is needed for camera movement logic and window size stats
    struct CameraTransformData
    {
        BlitML::mat4 rotation = BlitML::mat4();
        BlitML::mat4 translation = BlitML::mat4();

        // Keeps track of the movement of the camera
        BlitML::vec3 velocity = BlitML::vec3(0.f);

        // When this is 1 the UpdateCamera function will proceed normally
        uint8_t cameraDirty = 0;

        // Keep track of the rotation of the camera. 
        // When rotation occurs, they each generate a quaternion. Then the two quats change the roation matrix
        float yawRotation = 0.f;
        float pitchRotation = 0.f;

        float windowWidth;
        float windowHeight;
        uint8_t windowResize;

        // Projection matrix data
        float fov;
        BlitML::mat4 projectionMatrix;
        BlitML::mat4 projectionTranspose;

        uint8_t freezeFrustum = 0;
    };

    // This struct will hold data important for rendering (view frustum, projection matrix, view matrix), 
    // It needs to be alinged with what is passed to the shader
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

    // Temporary camera struct, I am going to make it more robust in the future
    struct Camera
    {
        CameraViewData viewData;

        CameraTransformData transformData;
    };

    // Gives some default values to a new camera so that it does not spawn with a random transform
    void SetupCamera(Camera& camera, float fov, float windowWidth, float windowHeight, float zNear, 
    BlitML::vec3 initialCameraPosition, float drawDistance, float initialYawRotation = 0, float initialPitchRotation = 0);

    // Moves a camera based on the velocity and the rotation matrix. Only works if cameraDirty is 1
    void UpdateCamera(Camera& camera, float deltaTime);

    // Rotates a camera based on the amount passed to pitchRotation and yawRotation. Does not support rollRotation for now
    void RotateCamera(Camera& camera, float deltaTime, float pitchRotation, float yawRotation);

    // Since the main camera is also responsible for the projection matrix, whenever it needs to be updated the main camera is passed to this functions
    // Values that have to do with projection are also updated
    void UpdateProjection(Camera& camera, float fov, float windowWidth, float windowHeight, float zNear);

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
        Camera cameraList[BLIT_MAX_CAMERAS_IN_SCENE];

        // The main camera is the one whose values are used for culling and other operations
        Camera& m_mainCamera;
    };
}