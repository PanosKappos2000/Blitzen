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
}tranformBuffer;

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

layout (std430, binding = 0) uniform CullingData
{
    // frustum planes
    float frustumRight;
    float frustumLeft;
    float frustumTop;
    float frustumBottom;

    float proj0;// The 1st element of the projection matrix
    float proj5;// The 12th element of the projection matrix

    // The draw distance and zNear, needed for both occlusion and frustum culling
    float zNear;
    float zFar;

    // Occulusion culling depth pyramid data
    float pyramidWidth;
    float pyramidHeight;

    float lodTarget;

    // Debug values
    uint occlusionEnabled;
    uint lodEnabled;

    uint drawCount;
}cullingData;


void main()
{
    uint objectId = gl_GlobalInvocationID.x;
    RenderObject object = objectBuffer.renders[objectId];
    Surface surface = surfaceBuffer.surfaces[object.surfaceId];

    indirect.draws[objectId].indexCount = surface.lod[0].indexCount;
    indirect.draws[objectId].instanceCount = 1;
    indirect.draws[objectId].firstIndex = surface.lod[0].firstIndex;
    indirect.draws[objectId].vertexOffset = surface.vertexOffset;
    indirect.draws[objectId].firstInstance = 0;
}