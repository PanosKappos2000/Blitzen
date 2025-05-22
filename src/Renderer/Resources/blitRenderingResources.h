#pragma once
#include "Mesh/blitMeshes.h"
#include "BlitCL/blitArray.h"
#include "Game/blitCamera.h"



namespace BlitzenEngine
{
    // Rendering resources container
    class RenderingResources
    {
    public:

        RenderingResources();

        RenderingResources operator = (const RenderingResources& rr) = delete;
        RenderingResources operator = (RenderingResources& rr) = delete;

        inline const TextureStats* GetTextureArrayPointer() 
        { 
            return m_textures; 
        }
        inline const TextureStats& GetTextureFromMap(const char* textureName) 
        { 
            return m_textureTable[textureName]; 
        }
        inline uint32_t GetTextureCount() const 
        { 
            return m_textureCount; 
        }
        inline const Material* GetMaterialArrayPointer() const 
        { 
            return m_materials; 
        }
        inline const Material& GetMaterialFromMap(const char* materialName) 
        { 
            return m_materialTable[materialName]; 
        }
        inline uint32_t GetMaterialCount() const 
        { 
            return m_materialCount; 
        }

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

        // These functions should be temporary
        inline void IncrementTextureCount()
        {
            ++m_textureCount;
        }
        inline uint32_t IncrementMaterialCount()
        {
            return ++m_materialCount;
        }

        void DefineMaterial(const BlitML::vec4& diffuseColor, float shininess, const char* diffuseMapName, const char* specularMapName, const char* materialName);

    private:

        MeshResources m_meshContext;

        // Holds all textures. No dynamic allocation.
        TextureStats m_textures[BlitzenCore::Ce_MaxTextureCount];
        BlitCL::HashMap<TextureStats> m_textureTable;
        uint32_t m_textureCount = 0;

        // Holds all materials. No dynamic allocation
        Material m_materials[BlitzenCore::Ce_MaxMaterialCount];
        BlitCL::HashMap<Material> m_materialTable;
        uint32_t m_materialCount = 0;
    };
    
}