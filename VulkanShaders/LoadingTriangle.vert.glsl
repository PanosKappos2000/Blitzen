#version 450
//#extension GL_EXT_debug_printf : enable

layout(push_constant) uniform constants 
{
    vec3 vertexColor;
};

layout(location = 0) out vec3 fragColor;

struct Vertex 
{
    vec3 pos;
    vec3 color;
};

vec3 gVertices[3] =
{
    vec3(0.0, -0.5, 0.0),
    vec3(0.5, 0.5, 0.0),
    vec3(-0.5, 0.5, 0.0)
};

void main() 
{
    gl_Position = vec4(gVertices[gl_VertexIndex], 1.0);
    fragColor = vertexColor;
}