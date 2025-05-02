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
struct Cluster
{
    // Bounding sphere for frustum culling
    vec3 center;
    float radius;

    // This is for backface culling
    int8_t coneAxisX;
    int8_t coneAxisY;
    int8_t coneAxisZ;
    int8_t coneCutoff;

    uint dataOffset; // Index into meshlet data
    uint8_t vertexCount;
    uint8_t triangleCount;
    uint8_t padding0;
    uint8_t padding1;
};

// The single buffer that holds all meshlet data in the scene
layout(set = 0, binding = 12, std430) readonly buffer ClusterBuffer
{
    Cluster clusters[];
}clusterBuffer;

layout(set = 0, binding = 13, std430) readonly buffer MeshletDataBuffer
{
    uint data[];
}meshletDataBuffer;

struct Lod
{
    // Non cluster path, used to create draw commands
    uint indexCount;
    uint firstIndex;

	// Cluster path, used to create draw commands
    uint clusterOffset;
    uint clusterCount;

    // Used for more accurate LOD selection
    float error;

    // Pad to 32 bytes total
    uint padding0;
    uint padding1;
    uint padding2;
};

layout(set = 0, binding = 4, std430) readonly buffer LodBuffer
{
    Lod levels[];
}lodBuffer;

struct Surface
{
    // Bounding sphere
    vec3 center;     
    float radius;            

    uint materialId;

    // Index to lod buffer
    // Each primitive can be drawn in multiple ways, depending on its LODs
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

struct IndirectTask
{
    uint taskId;

    uint groupCountX;
    uint groupCountY;
    uint groupCountZ;
};

// Indirect buffers are writeonly in compute and readonly in vertex
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
    layout(set = 0, binding = 7, std430) readonly buffer IndirectDrawBuffer
    {
        IndirectDraw draws[];
    }indirectDrawBuffer;

    layout(set = 0, binding = 8, std430) readonly buffer IndirectTasksBuffer
    {
        IndirectTask tasks[];
    }indirectTaskBuffer;
#endif

struct RenderObject
{
    // Index transform buffer
    uint meshInstanceId;
    // Index surface buffer
    uint surfaceId;
};

// TODO: Refactor the object buffers. There will be a single structure in the shader 
// and the correct one will be accessed based on the pointer passed to push contants
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

// The LOD index is calculated using a formula, 
// where the distance to the bounding sphere's surface is taken
// and the minimum error that would result in acceptable screen-space deviation
// is computed based on camera parameters
uint LODSelection(vec3 center, float radius, float scale, float lodTarget, uint lodOffset, uint lodCount)
{
    float distance = max(length(center) - radius, 0);
	float threshold = distance * lodTarget / scale;
    uint lodIndex = 0;
	for (uint i = 1; i < lodCount; ++i)
    {
		if (lodBuffer.levels[lodOffset + i].error < threshold)
        {
			lodIndex = i;
        }
    }
    return lodIndex;
}