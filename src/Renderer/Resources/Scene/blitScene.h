#pragma once
#include "Renderer/Resources/RenderObject/blitRender.h"
#include "Cgltf/cgltf.h"

namespace BlitzenEngine
{
    // Automatic free struct
    struct CgltfScope
    {
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