#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

struct Surface
{
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
    // Access to the render object of each commnad for the vertex shader
    uint objectId;

    // DRAW COMMAND
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

// Indirect buffers are writeonly in compute and readonly in vertex
#ifndef COMPUTE_PIPELINE
    layout(set = 0, binding = 7, std430) readonly buffer RWSSBO_DRAW_CMD
    {
        IndirectDraw data[];
    }rwssbo_DrawCmd;
#endif

struct RenderObject
{
    uint meshInstanceId;
    uint surfaceId;
};

layout(set = 0, binding = 8, std430) readonly buffer SSBO_RENDER
{
    RenderObject data[];
}ssbo_render;

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