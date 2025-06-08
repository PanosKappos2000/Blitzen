#pragma once
#include "Renderer/Resources/RenderObject/blitRender.h"
#include "Cgltf/cgltf.h"

namespace BlitzenEngine
{
    struct SceneContext
    {
        BlitCL::String m_name{ "" };

        BlitCL::DynamicArray<BlitCL::String> m_meshNames;

        BlitCL::DynamicArray<BlitCL::String> m_materialNames;

        BlitCL::DynamicArray<BlitCL::String> m_textureNames;

        uint32_t m_renderOffset;
        uint32_t m_renderCount{ 0 };

        uint32_t m_transparentRenderOffset;
        uint32_t m_transparentRenderCount{ 0 };

        inline const char* DBLOG() const
        {
            return m_name.GetClassic();
        }
    };

    // Automatic free struct
    struct CgltfScope
    {
        SceneContext* m_pScene{ nullptr };

        cgltf_data* pData;

        ~CgltfScope();
    };

    void RandomizeTransform(MeshTransform& transform, float multiplier, float scale);

    bool LoadGltfFile(const char* path, CgltfScope& cgltf);

    bool ModifyTextureFilepath(cgltf_texture* pTexture, const char* fullPath, std::string& texturePath);

    void LoadGltfMaterials(TextureManager& textureContext, const CgltfScope& cgltfScope, uint32_t previousTextureCount);

    void LoadGltfMeshes(MeshResources& meshContext, TextureManager& textureContext, const CgltfScope& cgltfScope, uint32_t previousMaterialCount, BlitCL::DynamicArray<uint32_t>& surfaceIndices);

    void LoadGltfMeshPrimitives(MeshResources& meshContext, TextureManager& textureContext, const CgltfScope& cgltfScope, const cgltf_mesh& gltfMesh, uint32_t previousMaterialCount);

    // Generates render objects for a gltf scene
    void LoadGltfNodes(RenderContainer& renders, MeshResources& meshContext, const CgltfScope& cgltfScope, const BlitCL::DynamicArray<uint32_t>& surfaceIndices);
}