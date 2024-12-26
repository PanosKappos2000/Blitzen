#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_shader_explicit_arithmetic_types : require

// Mutliple vertices are passed to the GPU for each surface, so that the surface can be drawn
struct Vertex
{
    // Assigned to gl_Position after being transformed to the right coordinates
    vec3 position;
    // Passed to the fragment shader for texture mapping
    float16_t uvX;
    // Passed to the fragment shader for lighting calculations
    vec3 normal;
    // Passed to the fragment shader for texture mapping
    float16_t uvY;
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
    // Indices to the index buffer to draw the meshlet
    uint vertices[64];
    // Indices into the above buffer
	uint indices[126 * 3]; // up to 42 triangles
	uint triangleCount;
	uint vertexCount;
};

// The single buffer that holds all meshlet data in the scene
layout(buffer_reference, std430) readonly buffer MeshBuffer
{
    Meshlet meshlets[];
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

// The initial indirect data that is loaded to the GPU from the CPU during loading. Will be used to fill the final indirect buffer
struct IndirectOffsets
{
    // Holds up to 8 level of detail data structs
    MeshLod lod[8];
    // TODO: this could easily be uint8
    // Indicates the active elements of the above array, the lod index needs to be clamped between 0 and this value
    uint lodCount;

    // The vertex offset is the same for every level of details and must be passed to the indirect commands buffer based on the renderer's structure
    uint vertexOffset;

    uint taskCount;
    uint firstTask;
};

// Draw indirect struct. Accessed by the vkCmdDrawIndexedIndirectCount command, but also written into by the culling compute shader
struct IndirectDraw
{
    // Since there will be fewer draw calls than the amount of objects in the scene, 
    // the indirect buffer will also hold the object ID to access per object data in the vertex shader
    uint objectId;

    // Everything needed for vkCmdDrawIndexedIndirectCount call
    uint indexCount;
    uint instanceCount;
    uint firstIndex;
    uint vertexOffset;
    uint firstInstance;

    uint taskCount;
    uint firstTask;
};

// This buffer holds the draw data for every object in the scene that might need to be drawn
layout(buffer_reference, std430) readonly buffer IndirectBuffer
{
    IndirectOffsets  offsets[];
};

// The below are the same buffer but it is defined differently in the compute pipeline
// This will be the final buffer used by vkCmdDrawIndexedIndirect and will be filled by a compute shader after doing culling and other operations
#ifdef COMPUTE_PIPELINE
    layout(buffer_reference, std430) writeonly buffer FinalIndirect
    {
        IndirectDraw indirectDraws[];
    };
#else
    // In the graphics pipeline, this needs to be accessed to retrieve the object ID
    layout(buffer_reference, std430) readonly buffer FinalIndirect
    {
        IndirectDraw indirectDraws[];
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

// The render object struct hold per object data, necessary for a draw call to actually place and draw the object correctly in a scene
struct RenderObject
{
    // Positions data(could be a model matrix as well but I like this)
    vec3 pos;
    float scale;
    vec4 orientation;

    // This data is used for boudning sphere frustum culling. It is only used by the culling compute shader. Could be placed elsewhere
    vec3 center;
    float radius;

    // Passed to the fragment shader to access the correct texture and material data
    uint materialTag;
};

// Holds an array of the above structs for every object in the scene. Passed to the GPU during load. Accessed using the objectId in the FinalIndirect buffer
layout(buffer_reference, std430) readonly buffer RenderObjectBuffer
{
    RenderObject renderObjects[];
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
};

layout (buffer_reference, std430) readonly buffer MaterialBuffer
{
    Material materials[];
};

// This holds global shader data passed to the GPU at the start of each frame
layout(set = 0, binding = 0) uniform ShaderData
{
    // Accessed by the compute shader to do frustum culling
    vec4 frustumData[6];

    // Used to transofrm objects to view and clip coordinates
    mat4 projection;
    mat4 view;
    // This is the result of the mulitplication of projection * view, to avoid calculating for every vertex shader invocation
    // It's possible that I might never need the two matrices above in the shader
    mat4 projectionView;

    vec4 sunColor;
    vec3 sunDir;

    vec3 viewPosition;// Needed for lighting calculations
}shaderData;

// Holds the address of every buffer that needs to be accessed in the shader and is passed to the GPU as uniform buffer at the start of each frame
layout(set = 0, binding = 1) uniform BufferAddrs
{
    // Global Vertex buffer address
    VertexBuffer vertexBuffer;
    // Render object buffer(accessed by vertex shader)
    RenderObjectBuffer renderObjects;
    // Material buffer (accessed by fragment shader)
    MaterialBuffer materialBuffer;
    // meshlet buffer (accessed by mesh shader if mesh shading is active)
    MeshBuffer meshBuffer;
    // Initial indirect data buffer (accessed by compute to create the final indirect buffer)
    IndirectBuffer indirectBuffer;
    // Accessed by compute, vertex shader and the cpu when command recording for vkCmdDrawIndexedIndirectCount
    FinalIndirect finalIndirectBuffer;
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