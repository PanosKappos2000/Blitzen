#include "blitCamera.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenEngine
{
    // TODO: This function is incomplete, there are more thing I should be stting up
    void SetupCamera(Camera& camera, float fov, float windowWidth, float windowHeight, float zNear, 
    BlitML::vec3 initialCameraPosition, float initialYawRotation /*=0*/, float initialPitchRotation /*=0*/)
    {
        // Initalize the projection matrix
        camera.projectionMatrix = 
        BlitML::InfiniteZPerspective(fov, windowWidth / windowHeight, zNear);

        // Save these values for later use
        camera.fov = fov;
        camera.zNear = zNear;
        camera.windowWidth = windowWidth;
        camera.windowHeight = windowHeight;

        // Setup the camera translation based on the initial position parameter
        camera.position = initialCameraPosition;
        camera.translation = BlitML::Translate(initialCameraPosition);

        // Find the yaw orientation of the camera using quaternions
        camera.yawRotation = initialYawRotation;
        BlitML::vec3 yAxis(0.f, -1.f, 0.f);
        BlitML::quat yawOrientation = BlitML::QuatFromAngleAxis(yAxis, camera.yawRotation, 0);

        // Find the pitch orientation of the camera using quaternions
        camera.pitchRotation = initialPitchRotation;
        BlitML::vec3 xAxis(1.f, 0.f, 0.f);
        BlitML::quat pitchOrientation = BlitML::QuatFromAngleAxis(xAxis, camera.pitchRotation, 0);

        // Turn the quaternions to matrices and multiply them to get the new rotation matrix
        BlitML::mat4 yawRot = BlitML::QuatToMat4(yawOrientation);
        BlitML::mat4 pitchRot = BlitML::QuatToMat4(pitchOrientation);
        camera.rotation = yawRot * pitchRot;

        camera.viewMatrix = BlitML::Mat4Inverse(camera.translation * camera.rotation);

        // This is calculated here so that the shaders don't need to calculate this for every invocation
        camera.projectionViewMatrix = camera.projectionMatrix * camera.viewMatrix;

        // The transpose of the projection matrix will be used for frustum culling
        camera.projectionTranspose = BlitML::Transpose(camera.projectionMatrix);
    }

    // This will move from here once I add a camera system
    void UpdateCamera(Camera& camera, float deltaTime)
    {
        if (camera.cameraDirty)
        {
            // I haven't overloaded the += operator
            BlitML::vec3 finalVelocity = camera.velocity * deltaTime * 40.f;
            BlitML::vec4 directionalVelocity = camera.rotation * BlitML::vec4(finalVelocity);
            camera.position = camera.position + BlitML::ToVec3(directionalVelocity);
            camera.translation = BlitML::Translate(camera.position);

            camera.viewMatrix = BlitML::Mat4Inverse(camera.translation * camera.rotation);
            camera.projectionViewMatrix = camera.projectionMatrix * camera.viewMatrix;
        }
    }

    void RotateCamera(Camera& camera, float deltaTime, float pitchRotation, float yawRotation)
    {
        // find how much the camera's yaw moved
        if(yawRotation < 100.f && yawRotation > -100.f)
        {
            camera.yawRotation += yawRotation * 10.f * deltaTime / 100.f;
        }
        // Find how much the camera's pitch moved
        if(pitchRotation < 100.f && pitchRotation > -100.f)
        {
            camera.pitchRotation -= pitchRotation * 10.f * deltaTime / 100.f;
        }

        // Find the yaw orientation of the camera using quaternions
        BlitML::vec3 yAxis(0.f, -1.f, 0.f);
        BlitML::quat yawOrientation = BlitML::QuatFromAngleAxis(yAxis, camera.yawRotation, 0);

        // Find the pitch orientation of the camera using quaternions
        BlitML::vec3 xAxis(1.f, 0.f, 0.f);
        BlitML::quat pitchOrientation = BlitML::QuatFromAngleAxis(xAxis, camera.pitchRotation, 0);

        // Turn the quaternions to matrices and multiply them to get the new rotation matrix
        BlitML::mat4 yawRot = BlitML::QuatToMat4(yawOrientation);
        BlitML::mat4 pitchRot = BlitML::QuatToMat4(pitchOrientation);
        camera.rotation = yawRot * pitchRot;
    }

    void UpdateProjection(Camera& camera, float fov, float windowWidth, float windowHeight, float zNear)
    {
        // Change the projection according to the parameters
        camera.projectionMatrix = BlitML::InfiniteZPerspective(fov, windowWidth/ windowHeight, zNear);

        // Update the values that depend on the projection matrix
        camera.projectionViewMatrix = camera.projectionMatrix * camera.viewMatrix;
        camera.projectionTranspose = BlitML::Transpose(camera.projectionMatrix);

        // Update these values so they can be accessed if the camera system needs them separately
        camera.windowWidth = windowWidth;
        camera.windowHeight = windowHeight;
        camera.fov = fov;
        camera.zNear = zNear;
    }
}