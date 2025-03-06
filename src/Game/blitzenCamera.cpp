#include "blitCamera.h"

namespace BlitzenEngine
{
    void SetupCamera(Camera& camera, float fov, float windowWidth, float windowHeight, float zNear, 
    BlitML::vec3 initialCameraPosition, float drawDistance, 
    float initialYawRotation /*=0*/, float initialPitchRotation /*=0*/)
    {
        // Save these values for later use
        camera.transformData.fov = fov;
        camera.viewData.zNear = zNear;
        // Draw distance is held by the camera
        camera.viewData.zFar = drawDistance;

        // Setup the camera translation based on the initial position parameter
        camera.viewData.position = initialCameraPosition;
        camera.transformData.translation = BlitML::Translate(initialCameraPosition);

        // Find the yaw orientation of the camera using quaternions
        camera.transformData.yawRotation = initialYawRotation;
        BlitML::vec3 yAxis(0.f, -1.f, 0.f);
        BlitML::quat yawOrientation = BlitML::QuatFromAngleAxis(yAxis, camera.transformData.yawRotation, 0);

        // Find the pitch orientation of the camera using quaternions
        camera.transformData.pitchRotation = initialPitchRotation;
        BlitML::vec3 xAxis(1.f, 0.f, 0.f);
        BlitML::quat pitchOrientation = BlitML::QuatFromAngleAxis(xAxis, camera.transformData.pitchRotation, 0);

        // Turn the quaternions to matrices and multiply them to get the new rotation matrix
        BlitML::mat4 yawRot = BlitML::QuatToMat4(yawOrientation);
        BlitML::mat4 pitchRot = BlitML::QuatToMat4(pitchOrientation);
        camera.transformData.rotation = yawRot * pitchRot;

        camera.viewData.viewMatrix = BlitML::Mat4Inverse(camera.transformData.translation * camera.transformData.rotation);

        UpdateProjection(camera, windowWidth, windowHeight);
    }

    // This will move from here once I add a camera system
    void UpdateCamera(Camera& camera, float deltaTime)
    {
        if (camera.transformData.cameraDirty)
        {
            // I haven't overloaded the += operator
            BlitML::vec3 finalVelocity = camera.transformData.velocity * deltaTime * 40.f;
            BlitML::vec4 directionalVelocity = camera.transformData.rotation * BlitML::vec4(finalVelocity);
            camera.viewData.position = camera.viewData.position + BlitML::ToVec3(directionalVelocity);
            camera.transformData.translation = BlitML::Translate(camera.viewData.position);

            camera.viewData.viewMatrix = BlitML::Mat4Inverse(camera.transformData.translation * camera.transformData.rotation);
            camera.viewData.projectionViewMatrix = camera.transformData.projectionMatrix * camera.viewData.viewMatrix;
        }
    }

    void RotateCamera(Camera& camera, float deltaTime, float pitchRotation, float yawRotation)
    {
        // find how much the camera's yaw moved
        if(yawRotation < 100.f && yawRotation > -100.f)
        {
            camera.transformData.yawRotation += yawRotation * 10.f * deltaTime / 100.f;
        }
        // Find how much the camera's pitch moved
        if(pitchRotation < 100.f && pitchRotation > -100.f)
        {
            camera.transformData.pitchRotation -= pitchRotation * 10.f * deltaTime / 100.f;
        }

        // Find the yaw orientation of the camera using quaternions
        BlitML::vec3 yAxis(0.f, -1.f, 0.f);
        BlitML::quat yawOrientation = BlitML::QuatFromAngleAxis(yAxis, camera.transformData.yawRotation, 0);

        // Find the pitch orientation of the camera using quaternions
        BlitML::vec3 xAxis(1.f, 0.f, 0.f);
        BlitML::quat pitchOrientation = BlitML::QuatFromAngleAxis(xAxis, camera.transformData.pitchRotation, 0);

        // Turn the quaternions to matrices and multiply them to get the new rotation matrix
        BlitML::mat4 yawRot = BlitML::QuatToMat4(yawOrientation);
        BlitML::mat4 pitchRot = BlitML::QuatToMat4(pitchOrientation);
        camera.transformData.rotation = yawRot * pitchRot;
    }

    void UpdateProjection(Camera& camera, float newWidth, float newHeight)
    {
        // Updates the values that made the projection matrix change
        camera.transformData.windowWidth = newWidth;
        camera.transformData.windowHeight = newHeight;

        // Changes the projection according to the parameters
        camera.transformData.projectionMatrix = BlitML::InfiniteZPerspective(
            camera.transformData.fov, 
            newWidth / newHeight, 
            camera.viewData.zNear
        );

        // Updates the view * projection matrix results, as the projection matrix changed
        camera.viewData.projectionViewMatrix = camera.transformData.projectionMatrix * camera.viewData.viewMatrix;

        // Updates the transpose of the projection as well
        camera.transformData.projectionTranspose = BlitML::Transpose(camera.transformData.projectionMatrix);

        BlitML::vec4 nearPlane = BlitML::NormalizePlane(
            camera.transformData.projectionTranspose.GetRow(4) + 
            camera.transformData.projectionTranspose.GetRow(3)
        );
        ObliqueNearPlaneClippingMatrixModification(
            camera.transformData.projectionMatrix, 
            camera.onbcProjectionMatrix, nearPlane
        );

        // Updates view frustum values as they are dependent on the projection matrix
        BlitML::vec4 frustumX = BlitML::NormalizePlane(
            camera.transformData.projectionTranspose.GetRow(3) + 
            camera.transformData.projectionTranspose.GetRow(0)
        ); // x + w < 0
        BlitML::vec4 frustumY = BlitML::NormalizePlane(
            camera.transformData.projectionTranspose.GetRow(3) + 
            camera.transformData.projectionTranspose.GetRow(1)
        );
        camera.viewData.frustumRight = frustumX.x;
        camera.viewData.frustumLeft = frustumX.z;
        camera.viewData.frustumTop = frustumY.y;
        camera.viewData.frustumBottom = frustumY.z;
    
        // Updates the the parts of the projection matrix needed for occlusion culling
        camera.viewData.proj0 = camera.transformData.projectionMatrix[0];
        camera.viewData.proj5 = camera.transformData.projectionMatrix[5];
    
        // Updates the lod target threshold multiplier, as it is also dependent on projection
        camera.viewData.lodTarget = (2 / camera.viewData.proj5) * (1.f / float(camera.transformData.windowHeight));
    }

    void ObliqueNearPlaneClippingMatrixModification(BlitML::mat4& proj, BlitML::mat4& res, 
    const BlitML::vec4& clipPlane)
    {
        // Sets the oblique near-plane clipping matrix to the original projection matrix initially
        res = proj;

        BlitML::vec4 q;

        // Calculates the clip-space corner point opposite the clipping plane
        //  and transforms it into camera space by multiplying it by the inverse of the projection matrix
        q.x = (BlitML::ClipSpaceSignGL(clipPlane.x) + proj[8]) / proj[0];
        q.y = (BlitML::ClipSpaceSignGL(clipPlane.y) + proj[9]) / proj[5];
        q.z = -1.0F;
        q.w = (1.0F + proj[10]) / proj[14];

        // Calculates the scaled plane vector
        BlitML::vec4 c = clipPlane * (2.f / BlitML::Dot(clipPlane, q));
    
        // Replaces the third row of the Oblique near plane clipping projection matrix
        res[8] = c.x;
        res[9] = c.y;
        res[10] = c.z + 1.0F;
        res[11] = c.w;
    }

    // Must Declare the static variable
    CameraSystem* CameraSystem::m_sThis;

    CameraSystem::CameraSystem()
        :m_mainCamera{cameraList[BLIT_MAIN_CAMERA_ID]} // The main camera is the first element in the camera list
    {
        m_sThis = this;
    }
}