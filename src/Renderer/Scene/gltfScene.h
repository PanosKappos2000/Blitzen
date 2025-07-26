#pragma once
#include "blitScene.h"
#include "BlitCL/BlitString.h"
#include "Cgltf/cgltf.h"
#include <string>

namespace BlitzenEngine
{
    // Automatic free struct
    struct CgltfScope
    {
        SceneContext* m_pScene{ nullptr };

        cgltf_data* pData;

        ~CgltfScope();
    };

    SCENE_CREATE_RES ManageGltf(const char* filepath, RenderingResources* pResources, WORLD_RESIDENTS* pWorldResidents, RendererPtrType pRenderer, SceneContext* pScene, 
        RenderingLoadingContextMesh& loadingContextMesh);

	bool LoadGltfFile(const char* path, CgltfScope& cgltf);

    bool ModifyTextureFilepath(cgltf_texture* pTexture, const char* fullPath, std::string& texturePath);

    void LoadGltfMaterials(TextureManager& textureContext, const CgltfScope& cgltfScope, uint32_t previousTextureCount);

    bool LoadGltfMeshes(MeshResources& meshContext, TextureManager& textureContext, const CgltfScope& cgltfScope, uint32_t previousMaterialCount, uint32_t* surfaceIndices, 
        RenderingLoadingContextMesh& loadingContextMesh);

    bool LoadGltfMeshPrimitives(MeshResources& meshContext, TextureManager& textureContext, const CgltfScope& cgltfScope, const cgltf_mesh& gltfMesh, uint32_t previousMaterialCount, 
        RenderingLoadingContextMesh& loadingContextMesh, uint32_t& meshNum);

    bool LoadGltfNodes(WORLD_RESIDENTS* pResidents, MeshResources& meshContext, const CgltfScope& cgltfScope, const BlitCL::DynamicArray<uint32_t>& meshIndices);
}