float3 RotateQuat(float3 v, float4 quat)
{
    return v + 2.0 * cross(quat.xyz, cross(quat.xyz, v) + quat.w * v);
}

float3 ModelSphereToViewSphere(float3 center, float4x4 view, float4 orientation, float3 position, float scale)
{
    float3 worldCenter = RotateQuat(center, orientation) * scale + position;
    return mul(view, float4(worldCenter, 1)).xyz;
}

// Frustum culling function
bool FrustumCheck(float3 center, float radius, float frustumRight, float frustumLeft, float frustumTop, float frustumBottom, float znear, float zfar)
{
	bool visible = true;

    // the left/top/right/bottom plane culling utilizes frustum symmetry to cull against two planes at the same time
    // Formula taken from Arseny Kapoulkine's Niagara renderer https://github.com/zeux/niagara
    // It is also referenced in VKguide's GPU driven rendering articles https://vkguide.dev/docs/gpudriven/compute_culling/
    visible = visible && center.z * frustumLeft - abs(center.x) * frustumRight > -radius;
	visible = visible && center.z * frustumBottom - abs(center.y) * frustumTop > -radius;
	// the near/far plane culling uses camera space Z directly
	visible = visible && center.z + radius > znear && center.z - radius < zfar;

	return visible;
}

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
bool ProjectSphere(float3 center, float radius, float znear, float P00, float P11, out float4 aabb)
{
    // Too close to the camera
	if (center.z < radius + znear)
    {
		return false;
    }

	float3 scaledCenter = mul(center, radius);
	float zProjector = center.z * center.z - radius * radius;

    // Projected sphere width
	float xProjRadius = sqrt(center.x * center.x + zProjector);
	float xMin = (xProjRadius * center.x - scaledCenter.z) / (xProjRadius * center.z + scaledCenter.x);
	float xMax = (xProjRadius * center.x + scaledCenter.z) / (xProjRadius * center.z - scaledCenter.x);

    // Projected sphere height
	float yProjRadius = sqrt(center.y * center.y + zProjector);
	float yMin = (yProjRadius * center.y - scaledCenter.z) / (yProjRadius * center.z + scaledCenter.y);
	float yMax = (yProjRadius * center.y + scaledCenter.z) / (yProjRadius * center.z - scaledCenter.y);

    // Conversion to aabb
	aabb = float4(xMin * P00, yMin * P11, xMax * P00, yMax * P11);
	aabb = mul(aabb.xwzy, float4(0.5f, -0.5f, 0.5f, -0.5f)) + float4(0.5f, 0.5f, 0.5f, 0.5f); // clip space -> uv space

	return true;
}

bool OcclusionCheck(float4 aabb, Texture2D<float4> depthPyramid, SamplerState dpSampler, float pyramidWidth, float pyramidHeight, float3 center, float radius, float zNear)
{
    // Width and height of aabb adjusted to pyramid scale
	float width = (aabb.z - aabb.x) * pyramidWidth;
	float height = (aabb.w - aabb.y) * pyramidHeight;

    // Finds the mip map level that will match the screen size of the sphere
	float level = floor(log2(max(width, height)));

	float depth = depthPyramid.SampleLevel(dpSampler, float2(aabb.xy + aabb.zw) * 0.5, level).x;

	float depthSphere = zNear / (center.z - radius);

	return depthSphere > depth;
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