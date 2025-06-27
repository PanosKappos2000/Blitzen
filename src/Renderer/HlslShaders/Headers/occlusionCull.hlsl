Texture2D<float4> tex_HiZMap : register(t5);

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
bool ProjectSphere(float3 center, float radius, float znear, float P00, float P11, out float4 aabb)
{
    // Too close to the camera
	if (center.z < radius + znear)
	{
		return false;
	}

	float3 scaledCenter = center * radius;
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
	aabb = aabb.xwzy * float4(0.5f, -0.5f, 0.5f, -0.5f) + float4(0.5f, 0.5f, 0.5f, 0.5f); // clip space -> uv space

	return true;
}

bool OcclusionCheck(float4 aabb, uint pyramidWidth, uint pyramidHeight, float3 center, float radius, float zNear)
{
    // Scales aabb to pyramid width and height, to get the desired pyramid level
	float width = (aabb.z - aabb.x) * pyramidWidth;
	float height = (aabb.w - aabb.y) * pyramidHeight;
	uint level = ceil(log2(max(width, height)));

	// Scales aab to this mip's level, to access the texture
	uint mipWidth = max(1u, pyramidWidth >> level);
	uint mipHeight = max(1u, pyramidHeight >> level);
	aabb.x *= mipWidth * 0.5f;
	aabb.y *= mipHeight * 0.5f;
	aabb.z *= mipWidth * 0.5f;
	aabb.w *= mipHeight * 0.5f;

	float depth = tex_HiZMap.Load(uint3(aabb.xy + aabb.zw, level)).r;

	float depthSphere = zNear / (center.z - radius);

	return depthSphere > depth;
}