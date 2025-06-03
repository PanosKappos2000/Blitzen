#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

// Mutliple vertices are passed to the GPU for each surface, so that the surface can be drawn
struct Vertex
{
    vec3 position;
    float uvX, uvY;
    uint8_t normalX, normalY, normalZ, normalW;
    uint8_t tangentX, tangentY, tangentZ, tangentW;
    uint padding0;
};

// This is the single vertex buffer for the main graphics pipeline, accessed by draw indirect through index offset, index count and vertex offset
layout(set = 0, binding = 1, std430) readonly buffer VertexBuffer
{
    // An array of vertices of undefined size
    Vertex vertices[];
}vertexBuffer;

struct Surface
{
    vec3 center;     
    float radius;            

    uint materialId;

    uint lodOffset;
    uint lodCount;

    uint vertexOffset; // Not used in the shaders but is useful on the CPU (NOT CRUCIAL)
};

layout(set = 0, binding = 2, std430) readonly buffer SurfaceBuffer
{
    Surface surfaces[];
}surfaceBuffer;

struct IndirectDraw
{
    // Helps vertex shader access the correct object in the RenderObject array
    uint objectId;

    // Indirect command data, set by the culling compute shaders
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

// Indirect buffers are writeonly in compute and readonly in vertex
#ifdef COMPUTE_PIPELINE
    layout(set = 0, binding = 7, std430) writeonly buffer IndirectDrawBuffer
    {
        IndirectDraw draws[];
    }indirectDrawBuffer;

#else
    layout(set = 0, binding = 7, std430) readonly buffer IndirectDrawBuffer
    {
        IndirectDraw draws[];
    }indirectDrawBuffer;
#endif

struct RenderObject
{
    // Index transform buffer
    uint meshInstanceId;
    // Index surface buffer
    uint surfaceId;
};

layout(buffer_reference, std430) readonly buffer RenderObjectBuffer
{
    RenderObject objects[];
};

struct Transform
{
    vec3 pos;
    float scale;
    vec4 orientation;
};

layout(set = 0, binding = 5, std430) readonly buffer TransformBuffer
{
    Transform instances[];
}transformBuffer;

layout(set = 0, binding = 0) uniform ViewData
{
    mat4 view;
    mat4 projectionView;
    vec3 position;

    float frustumRight;
    float frustumLeft;
    float frustumTop;
    float frustumBottom;

    float proj0;
    float proj5;

    float zNear;
    float zFar;

    float pyramidWidth;
    float pyramidHeight;

    float lodTarget;
}viewData;

// This function is used in every vertex shader invocation to give the object its orientation
vec3 RotateQuat(vec3 v, vec4 quat)
{
	return v + 2.0 * cross(quat.xyz, cross(quat.xyz, v) + quat.w * v);
}

// Struct used for mesh shaders
struct MeshTaskPayload
{
	uint drawId;
	uint meshletIndices[32];
};