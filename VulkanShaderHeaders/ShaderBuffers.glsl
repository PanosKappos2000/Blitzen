#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

// Mutliple vertices are passed to the GPU for each surface, so that the surface can be drawn
struct Vertex
{
    vec3 position;
    float16_t uvX, uvY;
    uint8_t normalX, normalY, normalZ;
};

// This is the single vertex buffer for the main graphics pipeline, accessed by draw indirect through index offset, index count and vertex offset
layout(buffer_reference, std430) readonly buffer VertexBuffer
{
    // An array of vertices of undefined size
    Vertex vertices[];
};

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
layout(buffer_reference, std430) readonly buffer MeshletBuffer
{
    Meshlet meshlets[];
};

layout(buffer_reference, std430) readonly buffer MeshletDataBuffer
{
    uint data[];
};

// Holds a specific level of detail's index offset and count (as well as the according data for mesh shaders)
struct MeshLod
{
    // These allow the compute shader to give the index count and index offset to draw indirect for this specific level of detail
    uint indexCount;
    uint firstIndex;

    uint meshletCount;
    uint firstMeshlet;
};

struct Surface
{
    // Holds up to 8 level of detail data structs
    MeshLod lod[8];
    // TODO: this could easily be uint8
    // Indicates the active elements of the above array, the lod index needs to be clamped between 0 and this value
    uint lodCount;

    // The vertex offset is the same for every level of details and must be passed to the indirect commands buffer based on the renderer's structure
    uint vertexOffset;

    // Meshlets
    uint taskCount;
    uint firstTask;

    // Bounding sphere
    vec3 center;
    float radius;

    uint materialTag;
    uint surfaceIndex;
};

layout(buffer_reference, std430) readonly buffer SurfaceBuffer
{
    Surface surfaces[];
};

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
    layout(buffer_reference, std430) writeonly buffer IndirectDrawBuffer
    {
        IndirectDraw draws[];
    };

    layout(buffer_reference, std430) writeonly buffer IndirectTasksBuffer
    {
        IndirectTask tasks[];
    };
#else
    // In the graphics pipeline, this needs to be accessed to retrieve the object ID
    layout(buffer_reference, std430) readonly buffer IndirectDrawBuffer
    {
        IndirectDraw draws[];
    };

    layout(buffer_reference, std430) readonly buffer IndirectTasksBuffer
    {
        IndirectTask tasks[];
    };
#endif

// The indirect count buffer holds a single integer that is the draw count for VkCmdDrawIndexedIndirectCount. 
// Will be incremented when necessary by a compute shader
layout(buffer_reference, std430) writeonly buffer IndirectCount
{
    uint drawCount;
};

layout(buffer_reference, std430) buffer VisibilityBuffer
{
    uint visibilities[];
};

// Every possible draw call has one of these structs
struct RenderObject
{
    uint meshInstanceId;// Index into the mesh instance buffer
    uint surfaceId;// Index into the surface buffer
};

// Holds an array of the above structs for every object in the scene. Passed to the GPU during load. Accessed using the objectId in the FinalIndirect buffer
layout(buffer_reference, std430) readonly buffer ObjectBuffer
{
    RenderObject objects[];
};

struct MeshInstance
{
    vec3 pos;
    float scale;
    vec4 orientation;
};

layout(buffer_reference, std430) readonly buffer MeshInstanceBuffer
{
    MeshInstance instances[];
};

// Holds data that defines the material of a surface
struct Material
{
    // Material constants used to render the as desired
    vec4 diffuseColor;
    float shininess;

    // Used to access each texture map
    uint diffuseMapIndex;
    uint specularMapIndex;

    uint materialId;
};

layout (buffer_reference, std430) readonly buffer MaterialBuffer
{
    Material materials[];
};

// This holds global shader data passed to the GPU at the start of each frame
layout(set = 0, binding = 0) uniform ShaderData
{
    // This is the result of the mulitplication of projection * view, to avoid calculating for every vertex shader invocation
    mat4 projectionView;
    mat4 view;

    vec4 sunColor;
    vec3 sunDir;

    vec3 viewPosition;// Needed for lighting calculations
}shaderData;

// Holds the address of every buffer that needs to be accessed in the shader and is passed to the GPU as uniform buffer at the start of each frame
layout(set = 0, binding = 1) uniform BufferAddrs
{
    // Global Vertex buffer address
    VertexBuffer vertexBuffer;

    // Render object buffer(accessed by vertex shader and compute shaders)
    ObjectBuffer objectBuffer;

    // Material buffer (accessed by fragment shader)
    MaterialBuffer materialBuffer;

    // meshlet buffer (accessed by mesh shader)
    MeshletBuffer meshletBuffer;

    // Meshlet data buffer (accessed by mesh shader)
    MeshletDataBuffer meshletDataBuffer;

    // Initial indirect data buffer (accessed by compute to create the final indirect buffer)
    SurfaceBuffer surfaceBuffer;

    // Accessed by compute and vertex shader
    MeshInstanceBuffer transformBuffer;

    // Accessed by compute, vertex shader and the cpu when command recording for vkCmdDrawIndexedIndirectCount
    IndirectDrawBuffer indirectDrawBuffer;

    // Accessed by compute, mesh shader, task shader and the cpu when command recording for vkCmdDrawMeshTasksIndirectCountEXT
    IndirectTasksBuffer indirectTaskBuffer;

    // Incremented by compute and used by vkCmdDrawIndexedIndirectCount
    IndirectCount indirectCount;
    
    // Accessed by compute, and each element switched to 1 or 0 based on culling test results
    VisibilityBuffer visibilityBuffer;
}bufferAddrs;

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