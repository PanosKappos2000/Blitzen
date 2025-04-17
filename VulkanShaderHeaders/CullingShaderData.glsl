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
	float width = (aabb.z - aabb.x) * viewData.pyramidWidth;
	float height = (aabb.w - aabb.y) * viewData.pyramidHeight;
    // Find the mip map level that will match the screen size of the sphere
	float level = floor(log2(max(width, height)));
	float depth = textureLod(depthPyramid, (aabb.xy + aabb.zw) * 0.5, level).x;
	float depthSphere = viewData.zNear / (center.z - radius);
	return depthSphere > depth;
}

struct ClusterDispatchCommand
{
    uint groupCountX;
    uint groupCountY;
    uint groupCountZ;
    uint padding0;     // Aligns next line to 16 bytes

    uint objectId;
    uint lodIndex;
    uint padding1;
    uint padding2;     // Ensures struct size is 32 bytes
};

#ifdef PRE_CLUSTER
layout(set = 0, binding = 16, std430) writeonly buffer IndirectClusterDispatch
{
	ClusterDispatchCommand commands[];
}indirectClusterDispatch;
#else
layout(set = 0, binding = 16, std430) readonly buffer IndirectClusterDispatch
{
	ClusterDispatchCommand commands[];
}indirectClusterDispatch;
#endif

layout(set = 0, binding = 16, std430) writeonly buffer IndirectClusterCount
{
	uint count;
}indirectClusterCount;

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

layout (push_constant) uniform CullingConstants
{
    RenderObjectBuffer renderObjectBufferAddress;
    uint drawCount;
    uint8_t postPass;
}cullPC;