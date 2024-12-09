#extension GL_EXT_buffer_reference2 : require

struct Vertex
{
    vec3 position;
    float uvX;
    vec3 normal;
    float uvY;
    vec4 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer
{
    Vertex vertices[];
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
    uint diffuseMapIndex;
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

    vec3 sunDir;
    vec4 sunColor;

    VertexBuffer vertexBuffer;
    RenderObjectBuffer renderObjects;
    MaterialBuffer materialBuffer;
}shaderData;