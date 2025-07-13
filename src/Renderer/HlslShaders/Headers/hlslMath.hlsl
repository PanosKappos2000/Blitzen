float3 RotateQuat(float3 v, float4 quat)
{
    return v + 2.0 * cross(quat.xyz, cross(quat.xyz, v) + quat.w * v);
}

float QuatNormal(float4 q)
{
    return sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

float4 NormalizeQuat(float4 q)
{
    float normal = QuatNormal(q);
    return float4(q.x / normal, q.y / normal, q.z / normal, q.w / normal);
}

float4 NormalizedQuatFromAngleAxis(float3 axis, float angle)
{
    const float HALF_ANGLE = 0.5f * angle;

    float s = sin(HALF_ANGLE);
    float c = cos(HALF_ANGLE);

    float4 q = float4(s * axis.x, s * axis.y, s * axis.z, c);

    return NormalizeQuat(q);
}

float4 MultiplyQuat(float4 q1, float4 q2)
{
    float4 res;

    res.x = q1.x * q2.w + q1.y * q2.z - q1.z * q2.y + q1.w * q2.x;
    res.y = -q1.x * q2.z + q1.y * q2.w + q1.z * q2.x + q1.w * q2.y;
    res.z = q1.x * q1.y - q1.y * q2.x + q1.z * q2.w + q1.w * q2.z;
    res.w = -q1.x * q2.x - q1.y * q2.y - q1.z * q2.z + q1.w * q2.w;

    return res;
}

float4x4 Mat4EulerY(float radians)
{
    float4x4 res;
    float c = cos(radians);
    float s = sin(radians);
    res[0][0] = c;
    res[1][0] = -s;
    res[2][0] = s;
    res[3][0] = c;
    return res;
}

float3 ModelSphereToViewSphere(float3 center, float4x4 view, float4 orientation, float3 position, float scale)
{
    float3 worldCenter = RotateQuat(center, orientation) * scale + position;
    return mul(view, float4(worldCenter, 1)).xyz;
}

// Performs frustum culling against an axis-aligned bounding sphere in camera space.
//
// The four frustum planes (left, right, top, bottom) are computed ahead of time from the projection matrix 
// using BlitML::ExtractFrustumPlanesForBMPR (see BlitML.h). These planes represent the slope of the frustum 
// in X/Z and Y/Z space — optimized for GPU culling and simplified to reduce divergence.
//
// The function uses Arseny Kapoulkine’s frustum cone test from the Niagara renderer:
//     https://github.com/zeux/niagara
// It is also explained in the GPU-driven rendering section of vkguide.dev:
//     https://vkguide.dev/docs/gpudriven/compute_culling/
//
// This test leverages frustum symmetry and projects the sphere center into the XZ and YZ planes.
// It checks whether the sphere lies within the visible cone defined by the left/right and top/bottom planes.
//
// Parameters:
// - center:  Position of the bounding sphere in camera space (float3).
// - radius:  Radius of the bounding sphere.
// - frustumRight:  Right slope boundary of the X/Z cone.
// - frustumLeft:   Left slope boundary of the X/Z cone.
// - frustumTop:    Top slope boundary of the Y/Z cone.
// - frustumBottom: Bottom slope boundary of the Y/Z cone.
// - znear/zfar:     Clipping range in camera space Z.
//
// Returns:
// - true if the sphere is at least partially inside the frustum
// - false if fully outside
//
bool FrustumCheck(float3 center, float radius, float frustumRight, float frustumLeft, float frustumTop, float frustumBottom, float znear, float zfar)
{
	bool visible = true;

    // Tests X/Z frustum cone (left & right combined)
    // Projects center into camera XZ, compares against slope-constrained cone
    visible = visible && center.z * frustumLeft - abs(center.x) * frustumRight > -radius;
    
    // Tests Y/Z frustum cone (top & bottom combined)
    // Same as above, but in YZ space
	visible = visible && center.z * frustumBottom - abs(center.y) * frustumTop > -radius;

	// Test near/far planes using Z range
	visible = visible && center.z + radius > znear && center.z - radius < zfar;

	return visible;
}

bool ClusterBackfaceCheck(float3 center, float radius, float3 coneAxis, float coneCutoff, float3 cameraPosition)
{
    // Calculate the direction from the camera to the cluster's center
    float3 toCenter = center - cameraPosition;
    toCenter = normalize(toCenter); // Normalize the direction vector
    
    // Computes the angle between the cone axis and the direction to the cluster
    float angle = acos(dot(toCenter, coneAxis));
    
    // If the angle is smaller than the cutoff, the cluster is visible
    return angle < coneCutoff;
}

float3 UnpackNormals(uint packed)
{
	float x = ((packed >> 24) & 0xFF) / 127.5f - 1.0f; 
    float y = ((packed >> 16) & 0xFF) / 127.5f - 1.0f;
    float z = ((packed >> 8) & 0xFF) / 127.5f - 1.0f;

    return float3(x, y, z); 
}

float4 UnpackTangents(uint packed)
{
	float x = ((packed >> 24) & 0xFF) / 127.5f - 1.0f; 
    float y = ((packed >> 16) & 0xFF) / 127.5f - 1.0f;
    float z = ((packed >> 8) & 0xFF) / 127.5f - 1.0f;
	float w = (packed & 0xFF) / 127.5f - 1.0f;

	return float4(x, y, z, w);
}

uint GetComputeShaderGroupSize(uint totalWorkCount, uint threadsPerGroup)
{
    return (totalWorkCount / threadsPerGroup) + 1;
}

float3 CalculateDirectionalLighting(float3 normal, float4 tangent, float3 normalMap)
{
    float3 bitangent = cross(normal, tangent.xyz) * tangent.w;
    float3 finalTangent = tangent.xyz - dot(tangent.xyz, normal) * normal;
    float3 nrm = normalize(normalMap.r * finalTangent + normalMap.g * bitangent + normalMap.b * normal);
    float3 sunDirection = normalize(float3(-1, 1, -1));
    return max(dot(nrm, sunDirection), 0.0);
}