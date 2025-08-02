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
        uint32_t textureCount = 0;
        RenderObject* renderObjects;
        MeshTransform* meshTransforms;

        ~CgltfScope();
    };

    BLIT_OFFLINE_FUNC SCENE_CREATE_RES ManageGltf(const char* filepath, const char* sceneName, RenderingResources* pResources, WORLD_RESIDENTS* pWorldResidents, RendererPtrType pRenderer);

	BLIT_OFFLINE_FUNC bool LoadGltfFile(const char* path, CgltfScope& cgltf);

    BLIT_OFFLINE_FUNC bool ModifyTextureFilepath(cgltf_texture* pTexture, const char* fullPath, std::string& texturePath);

    BLIT_OFFLINE_FUNC bool LoadGltfMaterials(MaterialManager& materialManager, CgltfScope& cgltfScope);

    BLIT_OFFLINE_FUNC bool LoadGltfMeshes(MeshResources& meshContext, TextureManager& textureContext, CgltfScope& cgltfScope, RenderingLoadingContextMesh& loadingContextMesh);

    BLIT_OFFLINE_FUNC bool LoadGltfMeshPrimitives(MeshResources& meshContext, TextureManager& textureContext, CgltfScope& cgltfScope, const cgltf_mesh& gltfMesh, 
        RenderingLoadingContextMesh& loadingContextMesh, uint32_t meshID);

    BLIT_OFFLINE_FUNC bool LoadGltfNodes(WORLD_RESIDENTS* pResidents, MeshResources& meshContext, CgltfScope& cgltfScope);
}