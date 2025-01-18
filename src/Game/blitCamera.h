#pragma once
#include "BlitzenMathLibrary/blitML.h"

#define BLIT_MAX_CAMERAS_IN_SCENE    2
#define BLIT_MAIN_CAMERA_ID          0
#define BLIT_DETATCHED_CAMERA_ID     1

namespace BlitzenEngine
{
    // Temporary camera struct, I am going to make it more robust in the future
    struct Camera
    {
        uint8_t cameraDirty = 0;// Tells the engine if the camera should be updated

        BlitML::mat4 viewMatrix;
        BlitML::mat4 projectionMatrix;
        BlitML::mat4 projectionViewMatrix;
        // Needed for frustum culling (probably does not need to be managed by the camera)
        BlitML::mat4 projectionTranspose;

        BlitML::vec3 position;

    
        float yawRotation = 0.f;
        float pitchRotation = 0.f;

        float fov;
        float zNear;

        float windowWidth;
        float windowHeight;

        BlitML::mat4 rotation = BlitML::mat4();
        BlitML::mat4 translation = BlitML::mat4();

        BlitML::vec3 velocity = BlitML::vec3(0.f);
    };

    void SetupCamera(Camera& camera, float fov, float windowWidth, float windowHeight, float zNear, 
    BlitML::vec3 initialCameraPosition, float initialYawRotation = 0, float initialPitchRotation = 0);
    // Will be called every frame to smoothly update camera based on user input
    void UpdateCamera(Camera& camera, float deltaTime);
    // Called directly from key press event functions to change the camera's orientation and rotation matrix
    void RotateCamera(Camera& camera, float deltaTime, float pitchRotation, float yawRotation);

    struct CameraContainer
    {
        Camera cameraList[BLIT_MAX_CAMERAS_IN_SCENE];

        uint8_t activeCameraID;
    };
}