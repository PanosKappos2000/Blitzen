#pragma once
#include "blitScene.h"
#include "BlitCL/BlitString.h"
#include "Cgltf/cgltf.h"
#include <string>

namespace BlitzenEngine
{
    // Automatic free struct
    class CgltfScope
    {
    public:
        const char* sceneName;
        cgltf_data* pData;
        uint32_t meshPrimitiveCount = 0;
        uint32_t residentCount = 0;
        RenderObject* renderObjects;
        MeshTransform* meshTransforms;

        ~CgltfScope();
    };

    SCENE_CREATE_RES ManageGltf(const char* filepath, const char* sceneName, RenderingResources* pResources, WORLD_RESIDENTS* pWorldResidents, RendererPtrType pRenderer);

	bool LoadGltfFile(const char* path, CgltfScope& cgltf);

    bool ModifyTextureFilepath(cgltf_texture* pTexture, const char* fullPath, std::string& texturePath);

    void LoadGltfMaterials(TextureManager& textureContext, CgltfScope& cgltfScope, uint32_t previousTextureCount);

    bool LoadGltfMeshes(MeshResources& meshContext, TextureManager& textureContext, CgltfScope& cgltfScope, RenderingLoadingContextMesh& loadingContextMesh);

    bool LoadGltfMeshPrimitives(MeshResources& meshContext, TextureManager& textureContext, CgltfScope& cgltfScope, const cgltf_mesh& gltfMesh, 
        RenderingLoadingContextMesh& loadingContextMesh, uint32_t meshID);

    bool LoadGltfNodes(WORLD_RESIDENTS* pResidents, MeshResources& meshContext, CgltfScope& cgltfScope);
}