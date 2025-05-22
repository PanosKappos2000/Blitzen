#pragma once
#include "Mesh/blitMeshes.h"
#include "BlitCL/blitArray.h"
#include "Game/blitCamera.h"

// TO BE REMOVED
#include "Cgltf/cgltf.h"


namespace BlitzenEngine
{
    // Rendering resources container
    class RenderingResources
    {
    public:

        RenderingResources();

        RenderingResources operator = (const RenderingResources& rr) = delete;
        RenderingResources operator = (RenderingResources& rr) = delete;

        inline const TextureStats* GetTextureArrayPointer() { return m_textures; }
        inline const TextureStats& GetTextureFromMap(const char* textureName) { return m_textureTable[textureName]; }
        inline uint32_t GetTextureCount() const { return m_textureCount; }
        inline const Material* GetMaterialArrayPointer() const { return m_materials; }
        inline const Material& GetMaterialFromMap(const char* materialName) { return m_materialTable[materialName]; }
        inline uint32_t GetMaterialCount() const { return m_materialCount; }

        // MESH RESOURCES
        inline MeshResources& GetMeshContext() 
        {
            return m_meshContext; 
        }
        inline const BlitCL::DynamicArray<PrimitiveSurface>& GetSurfaceArray() const
        {
            return m_meshContext.m_surfaces;
        }
        inline const BlitCL::DynamicArray<Vertex>& GetVerticesArray() const
        {
            return m_meshContext.m_vertices;
        }
        inline const BlitCL::DynamicArray<HlslVtx>& GetHlslVertices() const
        {
            return m_meshContext.m_hlslVtxs;
        }
        inline const BlitCL::DynamicArray<uint32_t>& GetIndicesArray() const
        {
           return m_meshContext.m_indices;
        }
        inline const BlitCL::DynamicArray<uint32_t>& GetPrimitiveVertexCounts() const
        {
            return m_meshContext.m_primitiveVertexCounts;
        }
        inline const BlitCL::DynamicArray<Cluster>& GetClusterArray() const
        {
            return m_meshContext.m_clusters;
        }
        inline const BlitCL::DynamicArray<uint32_t>& GetClusterIndices() const
        {
            return m_meshContext.m_clusterIndices;
        }
        inline const BlitCL::DynamicArray<IsPrimitiveTransparent>& GetSurfaceTransparencies() const
        {
            return m_meshContext.m_bTransparencyList;
        }
		inline const BlitCL::DynamicArray<LodData>& GetLodData() const
		{
			return m_meshContext.m_LODs;
		}
        inline const BlitCL::DynamicArray<LodInstanceCounter>& GetLODInstanceList() const
        {
            return m_meshContext.m_lodInstanceList;
        }


        inline const BlitCL::DynamicArray<RenderObject>& GetTranparentRenders() const
        {
            return m_transparentRenders;
        }

        inline void IncrementTextureCount()
        {
            ++m_textureCount;
        }
     
    /*
        Per instance data
    */

    private:
        MeshResources m_meshContext;
    public:
 
        BlitCL::DynamicArray<MeshTransform> transforms{ BlitzenCore::Ce_MaxDynamicObjectCount };
        uint32_t dynamicTransformCount{ 0 };

        // Holds all the render objects / primitives. They index into one primitive and one transform each
        RenderObject renders[BlitzenCore::Ce_MaxRenderObjects];
        uint32_t renderObjectCount{ 0 };

        RenderObject onpcReflectiveRenderObjects[BlitzenCore::Ce_MaxONPC_Objects];
        uint32_t onpcReflectiveRenderObjectCount{ 0 };

		inline MeshTransform& GetTransform(uint32_t id) const
		{
			return transforms[id];
		}


    public:

        // Takes a mesh id and adds a render object based on that ID and a transform
        uint32_t AddRenderObjectsFromMesh(uint32_t meshId, const BlitzenEngine::MeshTransform& transform, bool isDynamic);

        void DefineMaterial(const BlitML::vec4& diffuseColor, float shininess, const char* diffuseMapName, const char* specularMapName, const char* materialName);

        bool CreateRenderObject(uint32_t transformId, uint32_t surfaceId);

        // Lovely piece of debug helper
        void CreateSingleObjectForTesting();

        void LoadGltfMaterials(const cgltf_data* pGltfData, uint32_t previousMaterialCount, uint32_t previousTextureCount);

		void AddMeshesFromGltf(const cgltf_data* pGltfData, const char* path, uint32_t previousMaterialCount, BlitCL::DynamicArray<uint32_t>& surfaceIndices);

        void AddPrimitivesFromGltf(const cgltf_data* pGltfData, const cgltf_mesh& pGltfMesh, uint32_t previousMaterialCount);

        // Generates render objects for a gltf scene
        void CreateRenderObjectsFromGltffNodes(cgltf_data* pGltfData, const BlitCL::DynamicArray<uint32_t>& surfaceIndices);
        
    
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
        Per object data
    */
    private:

        BlitCL::DynamicArray<RenderObject> m_transparentRenders;
    };

    // TODO: THESE THINGS WILL BE IN SCENE.H
    void RandomizeTransform(MeshTransform& transform, float multiplier, float scale);
    void LoadGeometryStressTest(RenderingResources* pResources, float transformMultiplier);
    void CreateObliqueNearPlaneClippingTestObject(RenderingResources* pResources);
}