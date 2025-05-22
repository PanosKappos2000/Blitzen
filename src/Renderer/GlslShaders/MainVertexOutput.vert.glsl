#version 450

#extension GL_NV_gpu_shader5 : enable
#extension GL_KHR_vulkan_glsl : enable
#extension GL_ARB_shader_draw_parameters : enable
#extension GL_NV_uniform_buffer_std430_layout : enable

struct Vertex
{
    vec3 position;
    float16_t uvX, uvY;
    uint8_t normalX, normalY, normalZ, normalW;
    uint8_t tangentX, tangentY, tangentZ, tangentW;
};

layout(std430, binding = 5) readonly buffer VertexBuffer
{
    Vertex vertices[];
}vertexBuffer;

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

// This holds global shader data passed to the GPU at the start of each frame
layout(std430, binding = 0) uniform ViewData
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

// Sends the normal to the fragment shader
layout (location = 0) out vec3 fragNormal;

// This function is used in every vertex shader invocation to give the object its orientation
vec3 RotateQuat(vec3 v, vec4 quat)
{
	return v + 2.0 * cross(quat.xyz, cross(quat.xyz, v) + quat.w * v);
}

void main()
{
    Vertex vertex = vertexBuffer.vertices[gl_VertexID];
    Transform transform = transformBuffer.transforms[gl_DrawIDARB];

    gl_Position = viewData.projectionView * vec4(RotateQuat(
    vertex.position, transform.orientation) * transform.scale + transform.pos, 1);

    // Unpack surface normals
    vec3 normal = vec3(vertex.normalX, vertex.normalY, vertex.normalZ) / 127.0 - 1.0;
    // Pass the normal after promoting it to model coordinates
    fragNormal =  RotateQuat(normal, transform.orientation);
}