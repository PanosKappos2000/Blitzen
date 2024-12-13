#include "resourceLoading.h"

#define STB_IMAGE_IMPLEMENTATION
#include "VendorCode/stb_image.h"

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "objparser.h"// Using some help from Arseny Kapoulnike
#include "Meshoptimizer/meshoptimizer.h"

namespace BlitzenEngine
{
    uint8_t LoadResourceSystem(EngineResources& resources)
    {
        resources.textureTable.SetCapacity(BLIT_MAX_TEXTURE_COUNT);
        resources.materialTable.SetCapacity(BLIT_MAX_MATERIAL_COUNT);

        return 1;
    }



    /*---------------------------------
        Texture specific functions
    ----------------------------------*/

    uint8_t LoadTextureFromFile(EngineResources& resources, const char* filename, const char* texName)
    {
        TextureStats& current = resources.textures[resources.currentTextureIndex];

        current.pTextureData = stbi_load(filename, &(current.textureWidth), &(current.textureHeight), &(current.textureChannels), 4);

        uint8_t load = current.pTextureData != nullptr;

        // Go to the next element in the container, only if the texture was loaded successfully
        if(load)
        {
            current.textureTag = static_cast<uint32_t>(resources.currentTextureIndex);
            resources.textureTable.Set(texName, &current);
            resources.currentTextureIndex++;
        }
        // If the load failed give the default texture
        else
        {
            // TODO: Load the default texture so that the application does not fail
        }

        return load;
    }



    /*----------------------------------
        Material specific functions
    -----------------------------------*/

    void DefineMaterial(EngineResources& resources, BlitML::vec4& diffuseColor, float shininess, const char* diffuseMapName, 
    const char* specularMapName, const char* materialName)
    {
        MaterialStats& current = resources.materials[resources.currentMaterialIndex];

        current.diffuseColor = diffuseColor;
        current.shininess = shininess;
        current.diffuseMapName = diffuseMapName;
        current.diffuseMapTag = resources.textureTable.Get(diffuseMapName, &resources.textures[0])->textureTag;
        current.specularMapName = specularMapName;
        current.specularMapTag = resources.textureTable.Get(specularMapName, &resources.textures[0])->textureTag;
        current.materialTag = static_cast<uint32_t>(resources.currentMaterialIndex);
        resources.materialTable.Set(materialName, &current);
        resources.currentMaterialIndex++;
    }



    uint8_t LoadMeshFromObj(EngineResources& resources, const char* filename, uint8_t buildMeshlets /*= 0*/)
    {
        if(resources.currentMeshIndex > BLIT_MAX_MESH_COUNT)
        {
            BLIT_ERROR("Max mesh count: ( %i ) reached!", BLIT_MAX_MESH_COUNT)
            BLIT_INFO("If more objects are needed, increase the BLIT_MAX_MESH_COUNT macro before starting the loop")
            return 0;
        }

        ObjFile file;
        if(!objParseFile(file, filename))
            return 0;

        size_t indexCount = file.f_size / 3;

        /*!
            The below code can load a single mesh but it breaks if I try to load more than one
        */

        BlitCL::DynamicArray<BlitML::Vertex> vertices(indexCount);

        for(size_t i = 0; i < indexCount; ++i)
        {
            BlitML::Vertex& vtx = vertices[i];

            int32_t vertexIndex = file.f[i * 3 + 0];
		    int32_t vertexTextureIndex = file.f[i * 3 + 1];
		    int32_t vertexNormalIndex = file.f[i * 3 + 2];

            vtx.position.x = file.v[vertexIndex * 3 + 0];
		    vtx.position.y = file.v[vertexIndex * 3 + 1];
		    vtx.position.z = file.v[vertexIndex * 3 + 2];
		    vtx.normal.x = vertexNormalIndex < 0 ? 0.f : file.vn[vertexNormalIndex * 3 + 0];
		    vtx.normal.y = vertexNormalIndex < 0 ? 0.f : file.vn[vertexNormalIndex * 3 + 1];
		    vtx.normal.z = vertexNormalIndex < 0 ? 1.f : file.vn[vertexNormalIndex * 3 + 2];
		    vtx.uvX = vertexTextureIndex < 0 ? 0.f : file.vt[vertexTextureIndex * 3 + 0];
		    vtx.uvY = vertexTextureIndex < 0 ? 0.f : file.vt[vertexTextureIndex * 3 + 1];
        }

        BlitCL::DynamicArray<uint32_t> remap(indexCount);
		size_t vertexCount = meshopt_generateVertexRemap(remap.Data(), 0, indexCount, vertices.Data(), indexCount, sizeof(BlitML::Vertex));

        size_t previousIndexBufferSize = resources.indices.GetSize();
        size_t previousVertexBufferSize = resources.vertices.GetSize();
        resources.indices.Resize(previousIndexBufferSize + indexCount);
        resources.vertices.Resize(previousVertexBufferSize + vertexCount);

        meshopt_remapVertexBuffer(resources.vertices.Data() + previousVertexBufferSize, 
        vertices.Data(), indexCount, sizeof(BlitML::Vertex), remap.Data());
		meshopt_remapIndexBuffer(resources.indices.Data() + previousIndexBufferSize, 0, indexCount, remap.Data());

        PrimitiveSurface newSurface;
        newSurface.indexCount = static_cast<uint32_t>(indexCount);
        newSurface.firstIndex = static_cast<uint32_t>(previousIndexBufferSize);
        newSurface.pMaterial = resources.materialTable.Get("loaded_material", &resources.materials[0]);

        if(buildMeshlets)
        {
            BlitML::Meshlet meshlet = {};

            BlitCL::DynamicArray<uint32_t> meshletVertices(resources.vertices.GetSize() - previousVertexBufferSize);
            meshletVertices.Fill(UINT32_MAX);

            newSurface.firstMeshlet = resources.meshlets.GetSize();

            for (size_t i = previousIndexBufferSize; i < resources.indices.GetSize(); i += 3)
            {
                uint32_t vtxIndexA = resources.indices[i + 0];
                uint32_t vtxIndexB = resources.indices[i + 1];
                uint32_t vtxIndexC = resources.indices[i + 2];

                uint32_t& vtxA = meshletVertices[vtxIndexA];
                uint32_t& vtxB = meshletVertices[vtxIndexB];
                uint32_t& vtxC = meshletVertices[vtxIndexC];

                // If the current meshlet's vertex count + the vertices that are going to be added next is over the meshlet vertex limit, 
                // It gets added to the dynamic array and a new entry is created
                if (meshlet.vertexCount + (vtxA == UINT32_MAX) + (vtxB == UINT32_MAX) + (vtxC == UINT32_MAX) > 64 || meshlet.indexCount + 3 > 126)
                {
                    newSurface.meshletCount++;
                    resources.meshlets.PushBack(meshlet);

                    for (size_t j = 0; j < meshlet.vertexCount; ++j)
                        meshletVertices[meshlet.vertices[j]] = UINT32_MAX;

                    meshlet = {};
                }

                if (vtxA == UINT32_MAX)
                {
                    vtxA = meshlet.vertexCount;
                    meshlet.vertices[meshlet.vertexCount++] = vtxIndexA;
                }
                if (vtxB == UINT32_MAX)
                {
                    vtxB = meshlet.vertexCount;
                    meshlet.vertices[meshlet.vertexCount++] = vtxIndexB;
                }
                if (vtxC == UINT32_MAX)
                {
                    vtxC = meshlet.vertexCount;
                    meshlet.vertices[meshlet.vertexCount++] = vtxIndexC;
                }

                meshlet.indices[meshlet.indexCount++] = vtxA;
                meshlet.indices[meshlet.indexCount++] = vtxB;
                meshlet.indices[meshlet.indexCount++] = vtxC;
            }
        }

        resources.meshes[resources.currentMeshIndex].surfaces.PushBack(newSurface);
        ++resources.currentMeshIndex;

        return 1;
    }



    void LoadDefaultData(EngineResources& resources)
    {
        LoadMeshFromObj(resources, "Assets/Meshes/well.obj", 1);
    }
}