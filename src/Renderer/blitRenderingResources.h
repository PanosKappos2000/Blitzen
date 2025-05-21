#pragma once
#include "renderingResourcesTypes.h"
#include "BlitCL/blitArray.h"
#include "BlitCL/DynamicArray.h"
#include "BlitCL/blitHashMap.h"
#include "Game/blitCamera.h"
#include "Cgltf/cgltf.h"
// Algorithms for building meshlets, loading LODs, optimizing vertex caches etc.
// https://github.com/zeux/meshoptimizer
#include "Meshoptimizer/meshoptimizer.h"

namespace BlitzenEngine
{
    // Rendering resources container
    class RenderingResources
    {
    public:

        RenderingResources(void* rendererData);

        RenderingResources operator = (const RenderingResources& rr) = delete;
        RenderingResources operator = (RenderingResources& rr) = delete;

        inline const TextureStats* GetTextureArrayPointer() { return m_textures; }
        inline const TextureStats& GetTextureFromMap(const char* textureName) { return m_textureTable[textureName]; }
        inline uint32_t GetTextureCount() const { return m_textureCount; }
        inline const Material* GetMaterialArrayPointer() const { return m_materials; }
        inline const Material& GetMaterialFromMap(const char* materialName) { return m_materialTable[materialName]; }
        inline uint32_t GetMaterialCount() const { return m_materialCount; }
        inline const BlitCL::DynamicArray<PrimitiveSurface>& GetSurfaceArray() const
        {
            return m_surfaces;
        }
        inline const BlitCL::DynamicArray<Vertex>& GetVerticesArray() const
        {
            return m_vertices;
        }
        inline const BlitCL::DynamicArray<HlslVtx>& GetHlslVertices() const
        {
            return m_hlslVtxs;
        }
        inline const BlitCL::DynamicArray<uint32_t>& GetIndicesArray() const
        {
           return m_indices;
        }
        inline const BlitCL::DynamicArray<uint32_t>& GetPrimitiveVertexCounts() const
        {
            return m_primitiveVertexCounts;
        }
        inline const BlitCL::DynamicArray<Cluster>& GetClusterArray() const
        {
            return m_clusters;
        }
        inline const BlitCL::DynamicArray<uint32_t>& GetClusterIndices() const
        {
            return m_clusterIndices;
        }
        inline const BlitCL::DynamicArray<RenderObject>& GetTranparentRenders() const
        {
            return m_transparentRenders;
        }
		inline const BlitCL::DynamicArray<LodData>& GetLodData() const
		{
			return m_lods;
		}
        inline const BlitCL::DynamicArray<LodInstanceCounter>& GetLODInstanceList() const
        {
            return m_lodInstanceList;
        }
     
    /*
        Per instance data
    */
    public:
 
        BlitCL::DynamicArray<MeshTransform> transforms{ BlitzenCore::Ce_MaxDynamicObjectCount };
        uint32_t dynamicTransformCount{ 0 };

        // Holds all the render objects / primitives. They index into one primitive and one transform each
        RenderObject renders[BlitzenCore::Ce_MaxRenderObjects];
        uint32_t renderObjectCount{ 0 };

        RenderObject onpcReflectiveRenderObjects[BlitzenCore::Ce_MaxONPC_Objects];
        uint32_t onpcReflectiveRenderObjectCount{ 0 };
        
        // Holds the meshes that were loaded for the scene. Meshes are a collection of primitives.
        Mesh meshes[BlitzenCore::Ce_MaxMeshCount];
        BlitCL::HashMap<Mesh> meshMap;
        size_t meshCount = 0;


        struct IsPrimitiveTransparent
        {
            bool b = false;
        };
        BlitCL::DynamicArray<IsPrimitiveTransparent> bTransparencyList;

        inline const BlitCL::DynamicArray<IsPrimitiveTransparent>& GetSurfaceTransparencies() const
        {
            return bTransparencyList;
        }

		inline MeshTransform& GetTransform(uint32_t id) const
		{
			return transforms[id];
		}


    public:

        // Takes a mesh id and adds a render object based on that ID and a transform
        uint32_t AddRenderObjectsFromMesh(uint32_t meshId, const BlitzenEngine::MeshTransform& transform, bool isDynamic);

        // Takes a filepath and adds a mesh
        uint8_t LoadMeshFromObj(const char* filename, const char* meshName);

        void DefineMaterial(const BlitML::vec4& diffuseColor, float shininess, const char* diffuseMapName,
            const char* specularMapName, const char* materialName);

        bool CreateRenderObject(uint32_t transformId, uint32_t surfaceId);

        // Lovely piece of debug helper
        void CreateSingleObjectForTesting();

        void GenerateHlslVertices();

    public:

        // Load a texture from a specified file. Assumes DDS format
		template<class RENDERER>
		bool LoadTextureFromFile(const char* filename, const char* texName, RENDERER* pRenderer)
		{
			// Don't go over the texture limit, might want to throw a warning here
			if (m_textureCount >= BlitzenCore::Ce_MaxTextureCount)
			{
                BLIT_WARN("Max texture count: %i, has been reached", BlitzenCore::Ce_MaxTextureCount);
                BLIT_ERROR("Texture from file: %s, failed to load", filename);
				return 0;
			}

			auto& texture = m_textures[m_textureCount];
			// If texture upload to the renderer succeeds, the texture count is incremented and the function returns successfully
			if (pRenderer->UploadTexture(texture.pTextureData, filename))
			{
				m_textureCount++;
				return true;
			}
            else
            {
                BLIT_ERROR("Texture from file: %s, failed to load", filename);
                return false;
            }
		}

        // Loads a scene from gltf file
		template <class RENDERER>
        bool LoadGltfScene(const char* path, RENDERER* pRenderer)
        {
            if (renderObjectCount >= BlitzenCore::Ce_MaxRenderObjects)
            {
                BLIT_WARN("BLITZEN_MAX_DRAW_OBJECT already reached, no more geometry can be loaded. \
                    GLTF LOADING FAILED!");
                return false;
            }

            cgltf_options options{};

            cgltf_data* pData{ nullptr };

            auto res = cgltf_parse_file(&options, path, &pData);
            if (res != cgltf_result_success)
            {
                BLIT_WARN("Failed to load gltf file: %s", path)
                return false;
            }

            // Automatic free struct
            struct CgltfScope
            {
                cgltf_data* pData;
                inline ~CgltfScope() { cgltf_free(pData); }
            };
            CgltfScope cgltfScope{ pData };

            res = cgltf_load_buffers(&options, pData, path);
            if (res != cgltf_result_success)
            {
                return false;
            }
            

            res = cgltf_validate(pData);
            if (res != cgltf_result_success)
            {
                return false;
            }
            

            BLIT_INFO("Loading GLTF scene from file: %s", path);

            // Textures
            auto previousTextureCount = m_textureCount;
            BLIT_INFO("Loading textures");
            LoadGltfTextures(pData, previousTextureCount, path, pRenderer);

            // Materials
            BLIT_INFO("Loading materials");
            auto previousMaterialCount = m_materialCount;
            LoadGltfMaterials(pData, previousMaterialCount, previousTextureCount);

            // Meshes and primitives
            BlitCL::DynamicArray<uint32_t> surfaceIndices(pData->meshes_count);
            BlitCL::DynamicArray<uint8_t> surfaceTransparencyArray;
            BLIT_INFO("Loading meshes and primitives");
            for (size_t i = 0; i < pData->meshes_count; ++i)
            {
                const cgltf_mesh& gltfMesh = pData->meshes[i];

                auto firstSurface = static_cast<uint32_t>(m_surfaces.GetSize());

                auto& blitzenMesh = meshes[meshCount];
                blitzenMesh.firstSurface = firstSurface;
                blitzenMesh.surfaceCount = static_cast<uint32_t>(gltfMesh.primitives_count);
                blitzenMesh.meshId = static_cast<uint32_t>(meshCount);
                meshCount++;

                surfaceIndices[i] = firstSurface;

                AddPrimitivesFromGltf(pData, gltfMesh, previousMaterialCount);
            }


            // Render objects
            BLIT_INFO("Loading scene nodes");
            CreateRenderObjectsFromGltffNodes(pData, surfaceIndices);

            return true;
        }


    private:

        // Loads a single primitive and adds it to the global array
        void LoadPrimitiveSurface(BlitCL::DynamicArray<Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices);

        // Generates clusters for a given array of vertices and indices
        size_t GenerateClusters(BlitCL::DynamicArray<Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices, uint32_t vertexOffset);

        void GenerateTangents(BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices);

        // Generates LODs for the vertices of a given surface
        void AutomaticLevelOfDetailGenration(PrimitiveSurface& surface, BlitCL::DynamicArray<Vertex>& surfaceVertices, BlitCL::DynamicArray<uint32_t>& surfaceIndices);

        // Generates bounding sphere for primitive based on given vertices and indices
        void GenerateBoundingSphere(PrimitiveSurface& surface, BlitCL::DynamicArray<Vertex>& surfaceVertices, BlitCL::DynamicArray<uint32_t>& surfaceIndices);

        // Generates render objects for a gltf scene
        void CreateRenderObjectsFromGltffNodes(cgltf_data* pGltfData, const BlitCL::DynamicArray<uint32_t>& surfaceIndices);

        void AddPrimitivesFromGltf(const cgltf_data* pGltfData, const cgltf_mesh& pGltfMesh, uint32_t previousMaterialCount);

        void LoadGltfMaterials(const cgltf_data* pGltfData, uint32_t previousMaterialCount, uint32_t previousTextureCount);


        // Called from gltf loader to handle gltf textures. Templated because it needs the renderer
        template<class RENDERER>
        void LoadGltfTextures(const cgltf_data* pGltfData, uint32_t previousTextureCount, const char* gltfFilepath, RENDERER pRenderer)
        {
            for (size_t i = 0; i < pGltfData->textures_count; ++i)
            {
                auto pTexture = &pGltfData->textures[i];
                if (!pTexture->image)
                {
                    break;
                }

                auto pImage = pTexture->image;
                if (!pImage->uri)
                {
                    break;
                }

                std::string ipath = gltfFilepath;
                auto pos = ipath.find_last_of('/');
                if (pos == std::string::npos)
                {
                    ipath = "";
                }
                else
                {
                    ipath = ipath.substr(0, pos + 1);
                }

                std::string uri{ pImage->uri };
                uri.resize(cgltf_decode_uri(&uri[0]));
                auto dot = uri.find_last_of('.');

                if (dot != std::string::npos)
                {
                    uri.replace(dot, uri.size() - dot, ".dds");
                }

                auto path = ipath + uri;

                LoadTextureFromFile(path.c_str(), path.c_str(), pRenderer);
            }
        }
    
    /*
        Textures and materials
    */
    private:

        // Holds all textures. No dynamic allocation.
        TextureStats m_textures[BlitzenCore::Ce_MaxTextureCount];
        BlitCL::HashMap<TextureStats> m_textureTable;
        uint32_t m_textureCount = 0;

        // Holds all materials. No dynamic allocation
        Material m_materials[BlitzenCore::Ce_MaxMaterialCount];
        BlitCL::HashMap<Material> m_materialTable;
        uint32_t m_materialCount = 0;

    /*
        Per primitive data
    */
    private:

        BlitCL::DynamicArray<PrimitiveSurface> m_surfaces;
        BlitCL::DynamicArray<LodData> m_lods;
        BlitCL::DynamicArray<LodInstanceCounter> m_lodInstanceList;

        // Holds the vertex count of each primitive. 
        // This does not need to be passed to shader for now. 
        // But I do need it for ray tracing
        BlitCL::DynamicArray<uint32_t> m_primitiveVertexCounts;

        BlitCL::DynamicArray<Vertex> m_vertices;
        BlitCL::DynamicArray<HlslVtx> m_hlslVtxs;
        BlitCL::DynamicArray<uint32_t> m_indices;

        BlitCL::DynamicArray<Cluster> m_clusters;
        // Cluster index buffer
        BlitCL::DynamicArray<uint32_t> m_clusterIndices;
    
    /*
        Per object data
    */
    private:

        BlitCL::DynamicArray<RenderObject> m_transparentRenders;
    };

    // Sets random transform
    void RandomizeTransform(MeshTransform& transform, float multiplier, float scale);

    // Test functions
    void LoadTestGeometry(RenderingResources* pResources);
    void LoadGeometryStressTest(RenderingResources* pResources, float transformMultiplier);
    void CreateObliqueNearPlaneClippingTestObject(RenderingResources* pResources);
}