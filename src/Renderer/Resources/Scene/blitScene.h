#pragma once
#include "Renderer/Resources/RenderObject/blitRender.h"
#include "Cgltf/cgltf.h"

namespace BlitzenEngine
{
    void RandomizeTransform(MeshTransform& transform, float multiplier, float scale);

    void LoadGltfMaterials(RenderingResources* pResources, const cgltf_data* pGltfData, uint32_t previousMaterialCount, uint32_t previousTextureCount);

    void LoadGltfMeshes(MeshResources& meshContext, Material* pMaterials, const cgltf_data* pGltfData, 
        const char* path, uint32_t previousMaterialCount, BlitCL::DynamicArray<uint32_t>& surfaceIndices);

    void LoadGltfMeshPrimitives(MeshResources& meshContext, Material* pMaterials,  const cgltf_data* pGltfData, const cgltf_mesh& pGltfMesh, uint32_t previousMaterialCount);

    // Generates render objects for a gltf scene
    void LoadGltfNodes(RenderContainer& renders, MeshResources& meshContext, cgltf_data* pGltfData, const BlitCL::DynamicArray<uint32_t>& surfaceIndices);
}