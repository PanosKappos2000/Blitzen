#pragma once
#include "BlitzenMathLibrary/blitML.h"
#include "Core/blitzenContainerLibrary.h"
#include "Game/blitCamera.h"
#include <string>

namespace BlitzenEngine
{
    constexpr uint32_t ce_maxTextureCount = 5'000;

    constexpr uint32_t ce_maxMaterialCount = 10'000;

    constexpr uint8_t ce_primitiveSurfaceMaxLODCount = 8;

    constexpr uint32_t ce_maxMeshCount = 1'000'000; 
	constexpr const char* ce_defaultMeshName = "bunny";

    constexpr uint32_t ce_maxRenderObjects = 5'000'000;

    constexpr uint32_t ce_maxONPC_Objects = 100;

    #ifdef BLITZEN_CLUSTER_CULLING
        constexpr uint8_t ce_buildClusters = 1;
    #else
        constexpr uint8_t ce_buildClusters = 0;
    #endif 
    
    // Data for each texture
    struct TextureStats
    {
        std::string filepath;

        // TODO: this is no longer used, might want to remove later
        uint8_t* pTextureData;

        // This tag is for the shaders to know which image memory to access, when a material uses this texture
        uint32_t textureTag;
    };

    // Data for each vertex. Passed to the GPU
    struct alignas(16) Vertex
    {
        // Vertex position (3 floats)
        BlitML::vec3 position;
        // Uv maps (2 halfs floats)
        uint16_t uvX, uvY;
        // Vertex normals (4 8-bit integers)
        uint8_t normalX, normalY, normalZ, normalW;
        // Tangents (4 8-bit integers)
        uint8_t tangentX, tangentY, tangentZ, tangentW;
    };

    // Data for each meshlet. Passed to the GPU
    struct alignas(16) Meshlet
    {
        // Bounding sphere for frustum culling
    	BlitML::vec3 center;
    	float radius;

        // This is for backface culling
    	int8_t cone_axis[3];
    	int8_t cone_cutoff;

    	uint32_t dataOffset; // Index into meshlet data
    	uint8_t vertexCount;
    	uint8_t triangleCount;
    };

	// Data for each material. Passed to the GPU
    struct alignas(16) Material
    {
        // Not using these 2 right now, might remove them when I'm not bored, but they (or something similar) will be used in the future
        BlitML::vec4 diffuseColor;
        float shininess;

        uint32_t albedoTag;// Index into the texture array, for the albedo map of the material

        uint32_t normalTag;// Index into the texture array, for the normal map of the material

        uint32_t specularTag;// Index into the texture array, for the specular map of the material

        uint32_t emissiveTag;// Index into the texture array for the emissive map of the material

        // TODO: I need to try removing this, it's a waste of space
        uint32_t materialId;
    };

    // Mesh LOD data. An array of these is held by each primitive
    struct MeshLod
    {
        // Accesses index data
        uint32_t indexCount;
        uint32_t firstIndex;

        // Accesses meshlet data
        uint32_t meshletCount = 0;
        uint32_t firstMeshlet;

        // LOD error
        float error;
    };

    // Per primitive data. Passed to the GPU
    struct alignas(16) PrimitiveSurface
    {
        // Bounding sphere data, needed for frustum culling
        BlitML::vec3 center;
        float radius;

        MeshLod meshLod[ce_primitiveSurfaceMaxLODCount];// This is wasteful
        uint8_t lodCount = 0;

        uint32_t vertexOffset;

        uint32_t materialId;

        uint8_t postPass = 0;
    };

    // A mesh is a collection of one or more primitives.
    struct Mesh
    {
        uint32_t firstSurface;
        uint32_t surfaceCount = 0;

        uint32_t meshId;
    };

    // Each mesh has a different transform. Passed to the GPU. Accessed through render object
    struct alignas(16) MeshTransform
    {
        BlitML::vec3 pos;
        float scale;
        BlitML::quat orientation;
    };

    // Per render object Data. Passed to the GPU
    struct RenderObject
    {
        uint32_t transformId;
        uint32_t surfaceId;
    };



    /*
        Container struct that holds all loaded resources for the scene
    */
    class RenderingResources
    {
    public:

        RenderingResources::RenderingResources(void* rendererData);

        // Holds all textures. No dynamic allocation.
        TextureStats textures[ce_maxTextureCount];
        BlitCL::HashMap<TextureStats> textureTable;
        size_t textureCount = 0;

        // Holds all materials. No dynamic allocation. Includes hashmap for separate access
        Material materials[ce_maxMaterialCount];
        BlitCL::HashMap<Material> materialTable;
        size_t materialCount = 0;


        /*
            Per primitive data
        */
        // Holds all the primitives / surfaces
        BlitCL::DynamicArray<BlitzenEngine::PrimitiveSurface> surfaces;

        // Holds the vertex count of each primitive. This does not need to be passed to shader for now. But I do need it for ray tracing
        BlitCL::DynamicArray<uint32_t> primitiveVertexCounts;

        // Holds the vertices of all the primitives that were loaded
        BlitCL::DynamicArray<Vertex> vertices;

        // Holds the indices of all the primitives that were loaded
        BlitCL::DynamicArray<uint32_t> indices;

        // Holds all clusters for all the primitives that were loaded
        BlitCL::DynamicArray<Meshlet> meshlets;

        // Holds the meshlet indices to index into the clusters above
        BlitCL::DynamicArray<uint32_t> meshletData;


        /*
            Per instance data
        */
        // Holds the transforms of every render object / instance on the scene 
        BlitCL::DynamicArray<MeshTransform> transforms;

        // Holds all the render objects / primitives. They index into one primitive and one transform each
        RenderObject renders[ce_maxRenderObjects];
        uint32_t renderObjectCount = 0; 

        RenderObject onpcReflectiveRenderObjects[ce_maxONPC_Objects];
        uint32_t onpcReflectiveRenderObjectCount = 0;
        
        // Holds the meshes that were loaded for the scene. Meshes are a collection of primitives.
        Mesh meshes[ce_maxMeshCount];
        BlitCL::HashMap<Mesh> meshMap;
        size_t meshCount = 0;



    public:

        // Takes a mesh id and adds a render object based on that ID and a transform
		inline uint32_t AddRenderObjectsFromMesh(uint32_t meshId, 
            const BlitzenEngine::MeshTransform& transform)
		{
            BlitzenEngine::Mesh& mesh = meshes[meshId];
			if (renderObjectCount + mesh.surfaceCount >= ce_maxRenderObjects)
			{
				BLIT_ERROR("Adding renderer objects from mesh will exceed the render object count")
				return 0;
			}

            uint32_t transformId = static_cast<uint32_t>(transforms.GetSize());
            transforms.PushBack(transform);

			for (uint32_t i = mesh.firstSurface; i < mesh.firstSurface + mesh.surfaceCount; ++i)
			{
                RenderObject& object = renders[renderObjectCount++];

                object.surfaceId = i;
                object.transformId = transformId;
			}
			
			return transformId;
		}

        uint8_t LoadMeshFromObj(const char* filename, const char* meshName);
        // Takes a path to a gltf file and loads the resources needed to render the scene
        // This function uses the cgltf library to load a .glb or .gltf scene
        // The repository can be found on https://github.com/jkuhlmann/cgltf
        uint8_t LoadGltfScene(const char* path, void* rendererData);

    public:

		template<class RENDERER>
		bool LoadTextureFromFile(const char* filename, const char* texName,
			RENDERER* pRenderer)
		{
			// Don't go over the texture limit, might want to throw a warning here
			if (textureCount >= ce_maxTextureCount)
			{
				BLIT_WARN("Max texture count: %i, has been reached", ce_maxTextureCount)
				BLIT_ERROR("Texture from file: %s, failed to load", filename)
				return 0;
			}

			TextureStats& texture = textures[textureCount];
			// If texture upload to the renderer succeeds, the texture count is incremented and the function returns successfully
			if (pRenderer->UploadTexture(texture.pTextureData, filename))
			{
				textureCount++;
				return true;
			}
            else
            {
                BLIT_ERROR("Texture from file: %s, failed to load", filename)
                return false;
            }
		}

    public:

		inline static RenderingResources* GetRenderingResources() { return s_pResources; }

    private:

		static RenderingResources* s_pResources;
    };

    // Draw context needs to be given to draw frame function, so that it can update uniform values
    struct DrawContext
    {
        // The camera holds crucial data for the shaders. 
        // The structs in the shader try to be aligned with the structs in the camera
        BlitzenEngine::Camera* pCamera;

        RenderingResources* pResources;

        // Debug values
        uint8_t bOcclusionCulling;
        uint8_t bLOD;

        // Tells the renderer if it should do call Oblique near clipping shaders
        uint8_t bOnpc = 0;

        inline DrawContext(BlitzenEngine::Camera* pCam, 
            RenderingResources* pr, 
            uint8_t onpc = 0, uint8_t bOC = 1, uint8_t bLod = 1
        ) : pCamera(pCam), pResources(pr), bOcclusionCulling{bOC}, bLOD{bLod}, bOnpc{onpc} 
        {}
    };





    // Sets random transform
    inline void RandomizeTransform(MeshTransform& transform, float multiplier, float scale)
    {
        transform.pos = BlitML::vec3(
            (float(rand()) / RAND_MAX) * multiplier,//x 
            (float(rand()) / RAND_MAX) * multiplier,//y
            (float(rand()) / RAND_MAX) * multiplier
        );

        transform.scale = scale;

        transform.orientation = BlitML::QuatFromAngleAxis(
            BlitML::vec3(
                (float(rand()) / RAND_MAX) * 2 - 1, // x
                (float(rand()) / RAND_MAX) * 2 - 1, // y
                (float(rand()) / RAND_MAX) * 2 - 1
            ),
            BlitML::Radians((float(rand()) / RAND_MAX) * 90.f), // Angle 
            0
        );
    }

    void DefineMaterial(RenderingResources* pResources, const BlitML::vec4& diffuseColor, 
        float shininess, const char* diffuseMapName, 
        const char* specularMapName, const char* materialName);

    // Placeholder to load some default resources while testing the systems
    void LoadTestGeometry(RenderingResources* pResources);

    // Functions for testing aspects of the renderer
    void LoadGeometryStressTest(RenderingResources* pResources);
    void CreateObliqueNearPlaneClippingTestObject(RenderingResources* pResources);

    template<typename MT>
    void CreateDynamicObjectRendererTest(RenderingResources* pResources, MT* pManager)
    {
        constexpr uint32_t ce_objectCount = 1000;
        if (pResources->renderObjectCount + ce_objectCount > ce_maxRenderObjects)
        {
            BLIT_ERROR("Could not add dynamic object renderer test, object count exceeds limit")
                return;
        }

        for (size_t i = 0; i < ce_objectCount; ++i)
        {
            BlitzenEngine::MeshTransform transform;
            RandomizeTransform(transform, 100.f, 1.f);

            if (!pManager->AddObject<BlitzenEngine::ClientTest>(
                pResources, transform, 1, "kitten"
            ))
                return;
        }
    }

    // Takes the command line arguments to form a scene (this is pretty ill formed honestly)
    template<class RT, class MT>
    void CreateSceneFromArguments(int argc, char** argv,
        RenderingResources* pResources, RT* pRenderer, MT* pManager, uint8_t& bOnpc)
    {
        if (argc > 1)
        {
            LoadTestGeometry(pResources);
            #if defined(BLIT_DYNAMIC_OBJECT_TEST)
                CreateDynamicObjectRendererTest(pResources, pManager);
            #endif
            // Special argument. Loads heavy scene to stress test the culling
            if (strcmp(argv[1], "RenderingStressTest") == 0)
            {
                LoadGeometryStressTest(pResources);

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    pResources->LoadGltfScene(argv[i], pRenderer);
                }
            }

            // Special argument. Test oblique near-plane clipping technique. Not working yet.
            else if (strcmp(argv[1], "ONPC_ReflectionTest") == 0)
            {
                LoadTestGeometry(pResources);
                CreateObliqueNearPlaneClippingTestObject(pResources);
                bOnpc = 1;

                // The following arguments are used as gltf filepaths
                for (int32_t i = 2; i < argc; ++i)
                {
                    pResources->LoadGltfScene(argv[i], pRenderer);
                }
            }

            // If there are no special arguments everything is treated as a filepath for a gltf scene
            else
            {
                for (int32_t i = 1; i < argc; ++i)
                {
                    pResources->LoadGltfScene(argv[i], pRenderer);
                }
            }
        }
    }
}