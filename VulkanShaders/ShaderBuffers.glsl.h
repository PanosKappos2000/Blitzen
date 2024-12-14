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

struct RenderObject
{
    mat4 worldMatrix;

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
}bufferAddrs;

struct VertToFragData
{
    vec2 uv;
    vec3 normal;
    uint materialTag;
    vec3 modelPos;
};