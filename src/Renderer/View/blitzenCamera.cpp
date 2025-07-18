#include "blitCamera.h"
#include "Core/blitzenEngine.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Core/BlitzenWorld/blitzenUserInterface.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenEngine
{
    void CreateRotationMatrixFromPitchAndYawQuaternion(const BlitML::quat& pitchOrientation, const BlitML::quat& yawOrientation, BlitML::mat4& rotationMatrix)
    {
        auto yawRot = BlitML::QuatToMat4(yawOrientation);
        auto pitchRot = BlitML::QuatToMat4(pitchOrientation);
        rotationMatrix = yawRot * pitchRot;
    }

    void SetupCamera(Camera& camera, float fov, float windowWidth, float windowHeight, float zNear, const BlitML::vec3& initialCameraPosition, float drawDistance, 
        float initialYawRotation, float initialPitchRotation)
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
        SetupCamera(camera, BlitML::Radians(BlitzenCore::Ce_InitialFOV), float(BlitzenCore::Ce_InitialWindowWidth), float(BlitzenCore::Ce_InitialWindowHeight),
            BlitzenCore::Ce_Znear, BlitML::vec3{ BlitzenCore::Ce_InitialCameraX, BlitzenCore::Ce_initialCameraY, BlitzenCore::Ce_initialCameraZ },
            BlitzenCore::Ce_InitialDrawDistance, 0.f, 0.f);
    }

    void UpdateResidentAttachedCamera(Camera& camera, float deltaTime)
    {
        RotateResidentAttachedCamera(camera, deltaTime);

        // Receive the resident's current position. Since the camera is attached to it, it will use it for translation
        // The resident is assumed to have already taken care of direction
        // In the future, this design might need to be improved upon
        BlitML::vec3 position = GetResidentPosition(camera.attachmentSettings.residentID);

        float offsetX = camera.attachmentSettings.paddingFromResident.z * BlitML::Sin(camera.transformData.yawRotation);
        float offsetZ = camera.attachmentSettings.paddingFromResident.z * BlitML::Cos(camera.transformData.yawRotation);
		BlitML::vec3 finalPosition = position + BlitML::vec3{ offsetX, camera.attachmentSettings.paddingFromResident.y, offsetZ };

        camera.viewData.position = BlitML::ToVec3(finalPosition);

        // Creates translation
        camera.transformData.translation = BlitML::Translate(camera.viewData.position);

        // Recreation of view matrix
        camera.viewData.viewMatrix = BlitML::Mat4Inverse(camera.transformData.translation * camera.transformData.rotation);

        // Projection * view update
        camera.viewData.projectionViewMatrix = camera.transformData.projectionMatrix * camera.viewData.viewMatrix;
    }

    void RotateResidentAttachedCamera(Camera& camera, float deltaTime)
    {
        constexpr float DeltaTimeAlreadyOnPass1 = 1.f;

        constexpr float SavePitch = 89.f;

        camera.transformData.pitchRotation = BlitML::FClamp(camera.transformData.pitchRotation, BlitML::Radians(-SavePitch), BlitML::Radians(SavePitch));

        // New yaw pitch quat and rotation update
        auto yawOrientation = BlitML::QuatFromAngleAxis(BlitML::vec3(0.f, -1.f, 0.f), camera.transformData.yawRotation, 0);
        auto pitchOrientation = BlitML::QuatFromAngleAxis(BlitML::vec3(-1.f, 0.f, 0.f), camera.transformData.pitchRotation, 0);
        BlitzenEngine::CreateRotationMatrixFromPitchAndYawQuaternion(pitchOrientation, yawOrientation, camera.transformData.rotation);

        camera.transformData.yawMovement = 0.f;
        camera.transformData.pitchMovement = 0.f;
    }

    void UpdateCamera(Camera& camera, float deltaTime)
    {
        constexpr float SavePitch = 89.f;

        float yawMovement = BlitML::Radians(camera.transformData.yawMovement);
        float pitchMovement = BlitML::Radians(camera.transformData.pitchMovement);

        camera.transformData.yawRotation += yawMovement;
        camera.transformData.pitchRotation += pitchMovement;

        camera.transformData.pitchRotation = BlitML::FClamp(camera.transformData.pitchRotation, BlitML::Radians(-SavePitch), BlitML::Radians(SavePitch));

        // New yaw pitch quat and rotation update
        auto yawOrientation = BlitML::QuatFromAngleAxis(BlitML::vec3(0.f, -1.f, 0.f), camera.transformData.yawRotation, 0);
        auto pitchOrientation = BlitML::QuatFromAngleAxis(BlitML::vec3(-1.f, 0.f, 0.f), camera.transformData.pitchRotation, 0);
        BlitzenEngine::CreateRotationMatrixFromPitchAndYawQuaternion(pitchOrientation, yawOrientation, camera.transformData.rotation);

        camera.transformData.yawMovement = 0.f;
        camera.transformData.pitchMovement = 0.f;

        camera.transformData.translation = BlitML::Translate(camera.viewData.position);

        // Recreation of view matrix
        camera.viewData.viewMatrix = BlitML::Mat4Inverse(camera.transformData.translation * camera.transformData.rotation);

        // Projection * view update
        camera.viewData.projectionViewMatrix = camera.transformData.projectionMatrix * camera.viewData.viewMatrix;
    }

    void UpdateProjection(Camera& camera, float newWidth, float newHeight)
    {
        // Creates the projection matrix first
        camera.transformData.projectionMatrix = BlitML::InfiniteZPerspective(camera.transformData.fov, newWidth / newHeight, camera.viewData.zNear);

        camera.viewData.proj0 = camera.transformData.projectionMatrix[0];
        camera.viewData.proj5 = camera.transformData.projectionMatrix[5];

        // Updates the projection transpose mutliplication, since at least the projection part has been updated.
        camera.viewData.projectionViewMatrix = camera.transformData.projectionMatrix * camera.viewData.viewMatrix;
        
        // Gets transpose of the projection matrix, to put it in row major order. The transpose of the projection matrix will then be used to get the frustum planes.
        camera.transformData.projectionTranspose = BlitML::Transpose(camera.transformData.projectionMatrix);
        // Extract frustum planes
        BlitML::vec4 frustumPlanesBMPR = BlitML::ExtractFrustumPlanesForBMPR(camera.transformData.projectionTranspose);
        camera.viewData.frustumRight = frustumPlanesBMPR.x;
        camera.viewData.frustumLeft = frustumPlanesBMPR.w;
        camera.viewData.frustumTop = frustumPlanesBMPR.y;
        camera.viewData.frustumBottom = frustumPlanesBMPR.z;

        // This is a test for oblique near plane clipping. 
        // TODO: Deactivate when no objects that use it exist
        auto plane = BlitML::NormalizePlane(camera.transformData.projectionTranspose.GetRow(4) + camera.transformData.projectionTranspose.GetRow(0));
        ObliqueNearPlaneClippingMatrixModification(camera.transformData.projectionMatrix, camera.transformData.onbcProjectionMatrix, plane);
    
        // Updates the lod target threshold multiplier, as it is also dependent on projection
        camera.viewData.lodTarget = (2 / camera.viewData.proj5) * (1.f / newHeight);
    }

    void ObliqueNearPlaneClippingMatrixModification(BlitML::mat4& proj, BlitML::mat4& res, const BlitML::vec4& clipPlane)
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