#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

#define LOD_ENABLED
#define OCCLUSION_ENABLED

// 2D Polyhedral Bounds of a Clipped, Perspective-Projected 3D Sphere. Michael Mara, Morgan McGuire. 2013
// Basically, this will be used to turn each object's bounding spere into an AABB to be used for occlusion culling
bool projectSphere(vec3 c, float r, float znear, float P00, float P11, out vec4 aabb)
{
	if (c.z < r + znear)
		return false;

	vec3 cr = c * r;
	float czr2 = c.z * c.z - r * r;

	float vx = sqrt(c.x * c.x + czr2);
	float minx = (vx * c.x - cr.z) / (vx * c.z + cr.x);
	float maxx = (vx * c.x + cr.z) / (vx * c.z - cr.x);

	float vy = sqrt(c.y * c.y + czr2);
	float miny = (vy * c.y - cr.z) / (vy * c.z + cr.y);
	float maxy = (vy * c.y + cr.z) / (vy * c.z - cr.y);

	aabb = vec4(minx * P00, miny * P11, maxx * P00, maxy * P11);
	aabb = aabb.xwzy * vec4(0.5f, -0.5f, 0.5f, -0.5f) + vec4(0.5f); // clip space -> uv space

	return true;
}

// Frustum culling function
bool IsObjectInsideViewFrustum(out vec3 center, out float radius, // Modified bounding sphere values
	vec3 boundCenter, float boundRadius, // Bounding sphere values
	float scale, vec3 pos, vec4 orientation, // Transform
	mat4 view, // view matrix
	float frustumRight, float frustumLeft, float frustumTop, float frustumBottom, // View frustum planes
	float znear, float zfar)
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

bool OcclusionCullingPassed(vec4 aabb, sampler2D depthPyramid, float pyramidWidth, float pyramidHeight, vec3 center, float radius, float zNear)
{
	float width = (aabb.z - aabb.x) * pyramidWidth;
	float height = (aabb.w - aabb.y) * pyramidHeight;
    // Find the mip map level that will match the screen size of the sphere
	float level = floor(log2(max(width, height)));
	float depth = textureLod(depthPyramid, (aabb.xy + aabb.zw) * 0.5, level).x;
	float depthSphere = zNear / (center.z - radius);
	return depthSphere > depth;
}

struct Lod
{
    // Non cluster path, used to create draw commands
    uint indexCount;
    uint firstIndex;

	// Cluster path, used to create draw commands
    uint clusterOffset;
    uint clusterCount;

    // Used for more accurate LOD selection
    float error;

    // Pad to 32 bytes total
    uint padding0;
    uint padding1;
    uint padding2;
};

layout(set = 0, binding = 4, std430) readonly buffer LodBuffer
{
    Lod levels[];
}lodBuffer;

// The LOD index is calculated using a formula, 
// where the distance to the bounding sphere's surface is taken
// and the minimum error that would result in acceptable screen-space deviation
// is computed based on camera parameters
uint LODSelection(vec3 center, float radius, float scale, float lodTarget, uint lodOffset, uint lodCount)
{
    float distance = max(length(center) - radius, 0);
	float threshold = distance * lodTarget / scale;
    uint lodIndex = 0;
	for (uint i = 1; i < lodCount; ++i)
    {
		if (lodBuffer.levels[lodOffset + i].error < threshold)
        {
			lodIndex = i;
        }
    }
    return lodIndex;
}

struct ClusterDispatchData
{
    uint objectId;
    uint lodIndex;
    uint clusterId;
};

#ifdef PRE_CLUSTER
layout(buffer_reference, std430) writeonly buffer ClusterDispatchBuffer
{
	ClusterDispatchData data[];
};
#else
layout(buffer_reference, std430) readonly buffer ClusterDispatchBuffer
{
	ClusterDispatchData data[];
};
#endif

layout(buffer_reference, std430) writeonly buffer ClusterCountBuffer
{
	uint count;
};

// The indirect count buffer holds a single integer that is the draw count for VkCmdDrawIndexedIndirectCount. 
// Will be incremented when necessary by a compute shader
layout(set = 0, binding = 9, std430) writeonly buffer IndirectDrawCount
{
    uint drawCount;
}indirectDrawCountBuffer;

layout(set = 0, binding = 10, std430) buffer VisibilityBuffer
{
    uint visibilities[];
}visibilityBuffer;

#ifdef CLUSTER_CULLING
layout (push_constant) uniform PushConstants
{
    RenderObjectBuffer renderObjectBuffer;
    ClusterDispatchBuffer clusterDispatchBuffer;
    ClusterCountBuffer clusterCountBuffer;
    uint drawCount;
	uint padding0;
}pushConstant;
#else
layout (push_constant) uniform CullingConstants
{
    RenderObjectBuffer renderObjectBuffer;
    uint drawCount;
	uint padding0;
}pushConstant;
#endif