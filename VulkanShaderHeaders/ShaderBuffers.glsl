#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

// Mutliple vertices are passed to the GPU for each surface, so that the surface can be drawn
struct Vertex
{
    vec3 position;
    float16_t uvX, uvY;
    uint8_t normalX, normalY, normalZ, normalW;
    uint8_t tangentX, tangentY, tangentZ, tangentW;
};

// This is the single vertex buffer for the main graphics pipeline, accessed by draw indirect through index offset, index count and vertex offset
layout(set = 0, binding = 1, std430) readonly buffer VertexBuffer
{
    // An array of vertices of undefined size
    Vertex vertices[];
}vertexBuffer;

// Meshlet used in the mesh shader to draw a surface or mesh
struct Meshlet
{
    // Boudning sphere
    vec3 center;
    float radius;
    
    // This is for backface culling
    int8_t cone_axis[3];
    int8_t cone_cutoff;

    uint dataOffset; // dataOffset..dataOffset+vertexCount-1 stores vertex indices, indices are packed in 4b units after that
    uint8_t vertexCount;
    uint8_t triangleCount;
};

// The single buffer that holds all meshlet data in the scene
layout(set = 0, binding = 12, std430) readonly buffer MeshletBuffer
{
    Meshlet meshlets[];
}meshletBuffer;

layout(set = 0, binding = 13, std430) readonly buffer MeshletDataBuffer
{
    uint data[];
}meshletDataBuffer;

// Holds a specific level of detail's index offset and count (as well as the according data for mesh shaders)
struct MeshLod
{
    uint indexCount;
    uint firstIndex;
    float error;

    uint meshletOffset;
    uint meshletCount;

    uint padding0;
};

struct Surface
{
    vec3 center;     
    float radius;            

    uint vertexOffset;

    uint materialID;

    uint lodCount;       
    uint padding0;      
    MeshLod lod[8];
};

layout(set = 0, binding = 2, std430) readonly buffer SurfaceBuffer
{
    Surface surfaces[];
}surfaceBuffer;

// Draw indirect struct. Accessed by the vkCmdDrawIndexedIndirectCount command, but also written into by the culling compute shader
struct IndirectDraw
{
    /* 
        Since there will be fewer draw calls than the amount of objects in the scene, 
        the indirect buffer will also hold the object ID to access per object data in the vertex shader
    */
    uint objectId;

    // Everything needed for vkCmdDrawIndexedIndirectCount call
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;
};

struct IndirectTask
{
    uint taskId;

    uint groupCountX;
    uint groupCountY;
    uint groupCountZ;
};

// The below are the same buffer but it is defined differently in the compute pipeline
// This will be the final buffer used by vkCmdDrawIndexedIndirect and will be filled by a compute shader after doing culling and other operations
#ifdef COMPUTE_PIPELINE
    layout(set = 0, binding = 7, std430) writeonly buffer IndirectDrawBuffer
    {
        IndirectDraw draws[];
    }indirectDrawBuffer;

    layout(set = 0, binding = 8, std430) writeonly buffer IndirectTasksBuffer
    {
        IndirectTask tasks[];
    }indirectTaskBuffer;
#else
    // In the graphics pipeline, this needs to be accessed to retrieve the object ID
    layout(set = 0, binding = 7, std430) readonly buffer IndirectDrawBuffer
    {
        IndirectDraw draws[];
    }indirectDrawBuffer;

    layout(set = 0, binding = 8, std430) readonly buffer IndirectTasksBuffer
    {
        IndirectTask tasks[];
    }indirectTaskBuffer;
#endif

// Every possible draw call has one of these structs
struct RenderObject
{
    uint meshInstanceId;// Index into the mesh instance buffer
    uint surfaceId;// Index into the surface buffer
};

// Holds an array of the above structs for every object in the scene. Passed to the GPU during load. Accessed using the objectId in the FinalIndirect buffer
layout(set = 0, binding = 4, std430) readonly buffer ObjectBuffer
{
    RenderObject objects[];
}objectBuffer;

layout(buffer_reference, std430) readonly buffer RenderObjectBuffer
{
    RenderObject objects[];
};

layout(set = 0, binding = 14, std430) readonly buffer OnpcReflectiveObjectBuffer
{
    RenderObject objects[];
}onpcReflectiveObjectBuffer;

layout(set = 0, binding = 15, std430) readonly buffer TransparentObjectBuffer
{
    RenderObject objects[];
}transparentObjectBuffer;

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

// Holds data that defines the material of a surface
struct Material
{
    // Material constants used to render the as desired
    vec4 diffuseColor;
    float shininess;

    // Used to access each texture map
    uint albedoTag;
    uint normalTag;
    uint specularTag;
    uint emissiveTag;

    uint materialId;
};

layout (set = 0, binding = 6, std430) readonly buffer MaterialBuffer
{
    Material materials[];
}materialBuffer;

// Temporary camera struct, I am going to make it more robust in the future
layout(set = 0, binding = 0) uniform ViewData
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