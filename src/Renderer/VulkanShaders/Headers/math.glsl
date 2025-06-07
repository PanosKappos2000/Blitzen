// This function is used in every vertex shader invocation to give the object its orientation
vec3 RotateQuat(vec3 v, vec4 quat)
{
	return v + 2.0 * cross(quat.xyz, cross(quat.xyz, v) + quat.w * v);
}

bool ProjectSphere(vec3 c, float r, float znear, float P00, float P11, out vec4 aabb)
{
	if (c.z < r + znear)
	{
		return false;
	}

	vec3 cr = c * r;
	float czr2 = c.z * c.z - r * r;

	// Projected sphere width
	float vx = sqrt(c.x * c.x + czr2);
	float minx = (vx * c.x - cr.z) / (vx * c.z + cr.x);
	float maxx = (vx * c.x + cr.z) / (vx * c.z - cr.x);

	// Projected sphere height
	float vy = sqrt(c.y * c.y + czr2);
	float miny = (vy * c.y - cr.z) / (vy * c.z + cr.y);
	float maxy = (vy * c.y + cr.z) / (vy * c.z - cr.y);

	// Conversion to aabb
	aabb = vec4(minx * P00, miny * P11, maxx * P00, maxy * P11);
	aabb = aabb.xwzy * vec4(0.5f, -0.5f, 0.5f, -0.5f) + vec4(0.5f); // clip space -> uv space

	return true;
}

// Frustum culling function
bool CheckFrustum(out vec3 center, out float radius, vec3 boundCenter, float boundRadius, float scale, vec3 pos, vec4 orientation,
	mat4 view, float frustumRight, float frustumLeft, float frustumTop, float frustumBottom, float znear, float zfar)
{
	// Promotes the bounding sphere's center to model and the view coordinates (frustum culling will be done on view space)
    center = RotateQuat(boundCenter, orientation) * scale + pos;
    center = (view * vec4(center, 1)).xyz;
	radius = boundRadius * scale;
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

bool CheckOcclusion(vec4 aabb, sampler2D depthPyramid, float pyramidWidth, float pyramidHeight, vec3 center, float radius, float zNear)
{
	float width = (aabb.z - aabb.x) * pyramidWidth;
	float height = (aabb.w - aabb.y) * pyramidHeight;

    // Find the mip map level that will match the screen size of the sphere
	float level = floor(log2(max(width, height)));

	float depth = textureLod(depthPyramid, (aabb.xy + aabb.zw) * 0.5, level).x;
	
	float depthSphere = zNear / (center.z - radius);

	return depthSphere > depth;
}