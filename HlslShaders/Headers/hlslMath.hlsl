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
// Basically, this will be used to turn each object's bounding spere into an AABB to be used for occlusion culling
bool projectSphere(float3 c, float r, float znear, float P00, float P11, out float4 aabb)
{
	if (c.z < r + znear)
    {
		return false;
    }

	float3 cr = c * r;
	float czr2 = c.z * c.z - r * r;

	float vx = sqrt(c.x * c.x + czr2);
	float minx = (vx * c.x - cr.z) / (vx * c.z + cr.x);
	float maxx = (vx * c.x + cr.z) / (vx * c.z - cr.x);

	float vy = sqrt(c.y * c.y + czr2);
	float miny = (vy * c.y - cr.z) / (vy * c.z + cr.y);
	float maxy = (vy * c.y + cr.z) / (vy * c.z - cr.y);

	aabb = float4(minx * P00, miny * P11, maxx * P00, maxy * P11);
	aabb = aabb.xwzy * float4(0.5f, -0.5f, 0.5f, -0.5f) + float4(0.5f, 0.5f, 0.5f, 0.5f); // clip space -> uv space

	return true;
}

bool OcclusionCheck(float4 aabb, Texture2D<float4> depthPyramid, float pyramidWidth, float pyramidHeight, float3 center, float radius, float zNear)
{
	float width = (aabb.z - aabb.x) * pyramidWidth;
	float height = (aabb.w - aabb.y) * pyramidHeight;

    // Finds the mip map level that will match the screen size of the sphere
	float level = floor(log2(max(width, height)));
	float depth = depthPyramid.Load(int3((aabb.xy + aabb.zw) * 0.5, level)).x;
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