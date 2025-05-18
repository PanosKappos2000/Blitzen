#include "blitCamera.h"
#include "Engine/blitzenEngine.h"

namespace BlitzenEngine
{
    /*
        Helpers
    */
    static void CreateRotationMatrixFromPitchAndYawQuaternion(const BlitML::quat& pitchOrientation, const BlitML::quat& yawOrientation,
        BlitML::mat4& rotationMatrix)
    {
        auto yawRot = BlitML::QuatToMat4(yawOrientation);
        auto pitchRot = BlitML::QuatToMat4(pitchOrientation);
        rotationMatrix = yawRot * pitchRot;
    }

    void SetupCamera(Camera& camera, float fov, float windowWidth, float windowHeight, float zNear, 
    const BlitML::vec3& initialCameraPosition, float drawDistance, 
    float initialYawRotation /*=0*/, float initialPitchRotation /*=0*/)
    {
        camera.transformData.fov = fov;
        camera.viewData.zNear = zNear;
        camera.viewData.zFar = drawDistance;

        // Initial Camera translation
        camera.viewData.position = initialCameraPosition;
        camera.transformData.translation = BlitML::Translate(initialCameraPosition);

        // Initial Camera yaw
        camera.transformData.yawRotation = initialYawRotation;
        auto yawOrientation = BlitML::QuatFromAngleAxis(BlitML::vec3{ 0.f, -1.f, 0.f }, camera.transformData.yawRotation, 0);

        // Initial Camera pitch
        camera.transformData.pitchRotation = initialPitchRotation;
        auto pitchOrientation = BlitML::QuatFromAngleAxis(BlitML::vec3{ 1.f, 0.f, 0.f }, camera.transformData.pitchRotation, 0);

        // Combine for rotation
		CreateRotationMatrixFromPitchAndYawQuaternion(pitchOrientation, yawOrientation, camera.transformData.rotation);

        // View matrix
        camera.viewData.viewMatrix = BlitML::Mat4Inverse(camera.transformData.translation * camera.transformData.rotation);

        // Projection matrix
        UpdateProjection(camera, windowWidth, windowHeight);
    }

    void SetupCamera(Camera& camera)
    {
        SetupCamera(camera, BlitML::Radians(BlitzenEngine::ce_initialFOV), static_cast<float>(BlitzenEngine::ce_initialWindowWidth), static_cast<float>(BlitzenEngine::ce_initialWindowHeight),
            BlitzenEngine::ce_znear, BlitML::vec3{ BlitzenEngine::ce_initialCameraX, BlitzenEngine::ce_initialCameraY, BlitzenEngine::ce_initialCameraZ }, 
            BlitzenEngine::ce_initialDrawDistance);
    }

    void UpdateCamera(Camera& camera, float deltaTime)
    {
        if (camera.transformData.bCameraDirty)
        {
            // Calculates and adds velocity
            auto rawVelocity = camera.transformData.velocity * deltaTime * 40.f;
            auto directionalVelocity = camera.transformData.rotation * BlitML::vec4{ rawVelocity };
            camera.viewData.position = camera.viewData.position + BlitML::ToVec3(directionalVelocity);

            // Creates translation
            camera.transformData.translation = BlitML::Translate(camera.viewData.position);

            // Recreation of view matrix
            camera.viewData.viewMatrix = BlitML::Mat4Inverse(camera.transformData.translation * camera.transformData.rotation);

            // Projection * view update
            camera.viewData.projectionViewMatrix = camera.transformData.projectionMatrix * camera.viewData.viewMatrix;
        }
    }

    void RotateCamera(Camera& camera, float deltaTime, float pitchRotation, float yawRotation)
    {
        // Notify for update
        camera.transformData.bCameraDirty = true;

        // Limiter x
        if(yawRotation < 100.f && yawRotation > -100.f)
        {
            camera.transformData.yawRotation += yawRotation * 10.f * deltaTime / 100.f;
        }

        // Limiter y
        if(pitchRotation < 100.f && pitchRotation > -100.f)
        {
            camera.transformData.pitchRotation -= pitchRotation * 10.f * deltaTime / 100.f;
        }

        // New yaw pitch quat and rotation update
        auto yawOrientation = BlitML::QuatFromAngleAxis(BlitML::vec3(0.f, -1.f, 0.f), camera.transformData.yawRotation, 0);
        auto pitchOrientation = BlitML::QuatFromAngleAxis(BlitML::vec3(1.f, 0.f, 0.f), camera.transformData.pitchRotation, 0);
        CreateRotationMatrixFromPitchAndYawQuaternion(pitchOrientation, yawOrientation, camera.transformData.rotation);
    }

    void UpdateProjection(Camera& camera, float newWidth, float newHeight)
    {
        // New window sizes
        camera.transformData.windowWidth = newWidth;
        camera.transformData.windowHeight = newHeight;

        // Infinite z projection
        camera.transformData.projectionMatrix = BlitML::InfiniteZPerspective(camera.transformData.fov, newWidth / newHeight, camera.viewData.zNear);

        camera.viewData.proj0 = camera.transformData.projectionMatrix[0];
        camera.viewData.proj5 = camera.transformData.projectionMatrix[5];

        // Updates other matrices that are affected by the projection matrix
        camera.viewData.projectionViewMatrix = camera.transformData.projectionMatrix * camera.viewData.viewMatrix;
        camera.transformData.projectionTranspose = BlitML::Transpose(camera.transformData.projectionMatrix);

        // This is a test for oblique near plane clipping. 
        // TODO: Deactivate when no objects that use it exist
        auto plane = BlitML::NormalizePlane(camera.transformData.projectionTranspose.GetRow(4) + camera.transformData.projectionTranspose.GetRow(0));
        ObliqueNearPlaneClippingMatrixModification(camera.transformData.projectionMatrix, camera.onbcProjectionMatrix, plane);

        // Frustum planes are retrieved from the new projection matrix
        auto frustumX = BlitML::NormalizePlane(camera.transformData.projectionTranspose.GetRow(3) + camera.transformData.projectionTranspose.GetRow(0));
        auto frustumY = BlitML::NormalizePlane(camera.transformData.projectionTranspose.GetRow(3) + camera.transformData.projectionTranspose.GetRow(1));

        // Frustum planes. The near and far plane will use camera space Z directly
        camera.viewData.frustumRight = frustumX.x;
        camera.viewData.frustumLeft = frustumX.z;
        camera.viewData.frustumTop = frustumY.y;
        camera.viewData.frustumBottom = frustumY.z;
    
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
        auto c = clipPlane * (2.f / BlitML::Dot(clipPlane, q));
    
        // Replaces the third row of the Oblique near plane clipping projection matrix
        res[2] = c.x;
        res[6] = c.y;
        res[10] = c.z + 1.0F;
        res[14] = c.w;
    }
}