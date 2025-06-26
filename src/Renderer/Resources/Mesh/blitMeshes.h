#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "BlitCL/blitHashMap.h"
#include "blitMeshPrimitive.h"
#include "blitTriangle.h"
#include "blitClusters.h"

namespace BlitzenEngine
{
    struct MeshResources
    {
        Mesh m_meshes[BlitzenCore::Ce_MaxMeshCount];
        BlitCL::HashMap<Mesh> m_meshMap;
        uint32_t m_meshCount = 0;

        MeshPrimitivesContainer m_meshPrimitives{};
        PrimitiveContainer m_triangles{};
        ClusterContainer m_clusters{};

        uint32_t AddMesh(uint32_t firstSurface, uint32_t surfaceCount, const char* meshName = "BLIT_DO_NOT_ADD_TO_MESH_TABLE");
    };

    // Loads mesh from mesh obj. Returns its id or Ce_MaxMeshCount if error occurs
    uint32_t LoadMeshFromObj(MeshResources& context, const char* filename, const char* meshName);

    void InitializeMeshResourcesPointer_STATIC_ACCESS(MeshResources* ptr);

	Mesh& RequestMeshResources_STATIC_ACCESS(const char* meshName);

    BoundingSphere* GetBoundingSphereResources_STATIC_ACCESS(Mesh* pMesh);

    BlitzenCore::FAT_BOOL GetMeshPrimitiveTransparencyFlag_STATIC_ACCESS(uint32_t surfaceID);
}