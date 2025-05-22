#version 460

#extension GL_NV_gpu_shader5 : enable
#extension GL_NV_uniform_buffer_std430_layout : enable

layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

struct InidirectDrawCommand
{
    uint  indexCount;
    uint  instanceCount;
    uint  firstIndex;
    uint  vertexOffset;
    uint  firstInstance;
};

layout (std430, binding = 0) writeonly buffer IndirectDrawBuffer
{
    InidirectDrawCommand draws[];
}indirect;

struct Transform
{
    vec3 pos;
    float scale;
    vec4 orientation;
};

layout(std430, binding = 1) readonly buffer TransformBuffer
{
    Transform transforms[];
}transformBuffer;

struct MeshLod
{
    uint indexCount;
    uint firstIndex;

    uint meshletCount;
    uint firstMeshlet;

    float error;
};

struct Surface
{
    // Bounding sphere
    vec3 center;
    float radius;

    // Holds up to 8 level of detail data structs
    MeshLod lod[8];
    
    // Indicates the active elements of the above array, the lod index needs to be clamped between 0 and this value
    uint8_t lodCount;

    // The vertex offset is the same for every level of details and must be passed to the indirect commands buffer based on the renderer's structure
    uint vertexOffset;

    uint materialTag;
};

layout(std430, binding = 2) readonly buffer SurfaceBuffer
{
    Surface surfaces[];
}surfaceBuffer;

struct RenderObject
{
    uint transformId;// Index into the mesh instance buffer
    uint surfaceId;// Index into the surface buffer
};

layout(std430, binding = 3) readonly buffer RenderObjectBuffer
{
    RenderObject renders[];
}objectBuffer;

layout (std430, binding = 0) uniform ViewData
{
    // The view matrix is the most important responsibility of the camera and crucial for rendering
    mat4 view;

    // The result of projection * view, recalculated when either of the values changes
    mat4 projectionView;

    // Position of the camera, used to change the translation matrix which becomes part of the view matrix
    vec3 position;

    float frustumRight;
    float frustumLeft;
    float frustumTop;
    float frustumBottom;

    float proj0;// The 1st element of the projection matrix
    float proj5;// The 12th element of the projection matrix

    // The values below are used to create the projection matrix. Stored to recreate the projection matrix if necessary
    float zNear;
    float zFar;

    float pyramidWidth;
    float pyramidHeight;

    float lodTarget;
}viewData;

layout(std430, binding = 1) uniform CullingData
{
    uint32_t drawCount;
    uint8_t occlusionEnabled;
    uint8_t lodEnabled;
}cullingData;

// This function is used in every vertex shader invocation to give the object its orientation
vec3 RotateQuat(vec3 v, vec4 quat)
{
	return v + 2.0 * cross(quat.xyz, cross(quat.xyz, v) + quat.w * v);
}

void main()
{
    uint objectId = gl_GlobalInvocationID.x;

    if(objectId > cullingData.drawCount)
        return;

    RenderObject object = objectBuffer.renders[objectId];
    Surface surface = surfaceBuffer.surfaces[object.surfaceId];
    Transform transform = transformBuffer.transforms[object.transformId];

    // Get the center of the bounding sphere and project it to view coordinates
    vec3 center = RotateQuat(surface.center, transform.orientation) * transform.scale + transform.pos;
    center = (viewData.view * vec4(center, 1)).xyz;

    // Get the radius of the bounding sphere and scale it
    float radius = surface.radius * transform.scale;

    // Check that the bounding sphere is inside the view frustum(frustum culling)
	bool visible = true;
    // the left/top/right/bottom plane culling utilizes frustum symmetry to cull against two planes at the same time
    // Formula taken from Arseny Kapoulkine's Niagara renderer https://github.com/zeux/niagara
    // It is also referenced in VKguide's GPU driven rendering articles https://vkguide.dev/docs/gpudriven/compute_culling/
    visible = visible && center.z * viewData.frustumLeft - abs(center.x) * viewData.frustumRight > -radius;
	visible = visible && center.z * viewData.frustumBottom - abs(center.y) * viewData.frustumTop > -radius;
	// the near/far plane culling uses camera space Z directly
	visible = visible && center.z + radius > viewData.zNear && center.z - radius < viewData.zFar;

    uint lodIndex = 0;
    if(int(cullingData.lodEnabled) == 1)
    {
        // Gets the distance from the camera
        float distance = max(length(center) - radius, 0);
        // Create the threshold taking into account the lodTarget and the object's scale
        float threshold = distance * viewData.lodTarget / transform.scale;
        // Get the biggest reduction lod whose error is less than the calculated threshold
        for(uint i = 0; i < surface.lodCount; ++i)
        {
            if(threshold > surface.lod[i].error)
                lodIndex = i;
        }
    }
    MeshLod lod = surface.lod[lodIndex];

    indirect.draws[objectId].indexCount = lod.indexCount;
    indirect.draws[objectId].instanceCount = visible ? 1 : 0;
    indirect.draws[objectId].firstIndex = lod.firstIndex;
    indirect.draws[objectId].vertexOffset = surface.vertexOffset;
    indirect.draws[objectId].firstInstance = 0;
}