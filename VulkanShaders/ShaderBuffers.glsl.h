#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

struct Vertex
{
    vec3 position;
    float16_t uvX;
    vec3 normal;
    float16_t uvY;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
};

struct Meshlet
{
    uint vertices[64];
	uint indices[126 * 3]; // up to 42 triangles
	uint triangleCount;
	uint vertexCount;
};

layout(buffer_reference, std430) readonly buffer MeshBuffer
{
    Meshlet meshlets[];
};

struct IndirectDraw
{
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;

    uint taskCount;
    uint firstTask;

    float padding;
};

layout(buffer_reference, std430) readonly buffer IndirectBuffer
{
    IndirectDraw indirectDraws[];
};

layout(buffer_reference, std430) writeonly buffer FinalIndirect
{
    IndirectDraw indirectDraws[];
};

struct RenderObject
{
    vec3 pos;
    float scale;
    vec4 orientation;

    vec3 center;
    float radius;

    uint materialTag;
};

layout(buffer_reference, std430) readonly buffer RenderObjectBuffer
{
    RenderObject renderObjects[];
};

struct Material
{
    vec4 diffuseColor;
    float shininess;

    uint diffuseMapIndex;
    uint specularMapIndex;
};

layout (buffer_reference, std430) readonly buffer MaterialBuffer
{
    Material materials[];
};

layout(set = 0, binding = 0) uniform ShaderData
{
    vec4 frustumData[6];

    mat4 projection;
    mat4 view;
    mat4 projectionView;

    vec4 sunColor;
    vec3 sunDir;

    vec3 viewPosition;// Needed for lighting calculations
}shaderData;

layout(set = 0, binding = 1) uniform BufferAddrs
{
    VertexBuffer vertexBuffer;
    RenderObjectBuffer renderObjects;
    MaterialBuffer materialBuffer;
    MeshBuffer meshBuffer;
    IndirectBuffer indirectBuffer;
    FinalIndirect finalIndirectBuffer;
}bufferAddrs;

struct VertToFragData
{
    vec2 uv;
    vec3 normal;
    uint materialTag;
    vec3 modelPos;
};

vec3 RotateQuat(vec3 v, vec4 quat)
{
	return v + 2.0 * cross(quat.xyz, cross(quat.xyz, v) + quat.w * v);
}