#include "resourceLoading.h"

#define STB_IMAGE_IMPLEMENTATION
#include "VendorCode/stb_image.h"

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "objparser.h"// Using some help from Arseny Kapoulkine
#include "Meshoptimizer/meshoptimizer.h"

// Right now I don't know if I should rely on this or my own math library
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include <iostream>

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

        BlitCL::DynamicArray<BlitML::Vertex> triangleVertices(indexCount);

        for(size_t i = 0; i < indexCount; ++i)
        {
            BlitML::Vertex& vtx = triangleVertices[i];

            int32_t vertexIndex = file.f[i * 3 + 0];
		    int32_t vertexTextureIndex = file.f[i * 3 + 1];
		    int32_t vertexNormalIndex = file.f[i * 3 + 2];

            vtx.position.x = file.v[vertexIndex * 3 + 0];
		    vtx.position.y = file.v[vertexIndex * 3 + 1];
		    vtx.position.z = file.v[vertexIndex * 3 + 2];
		    vtx.normal.x = vertexNormalIndex < 0 ? 0.f : file.vn[vertexNormalIndex * 3 + 0];
		    vtx.normal.y = vertexNormalIndex < 0 ? 0.f : file.vn[vertexNormalIndex * 3 + 1];
		    vtx.normal.z = vertexNormalIndex < 0 ? 1.f : file.vn[vertexNormalIndex * 3 + 2];
		    vtx.uvX = meshopt_quantizeHalf(vertexTextureIndex < 0 ? 0.f : file.vt[vertexTextureIndex * 3 + 0]);
		    vtx.uvY = meshopt_quantizeHalf(vertexTextureIndex < 0 ? 0.f : file.vt[vertexTextureIndex * 3 + 1]);
        }

        BlitCL::DynamicArray<uint32_t> remap(indexCount);
		size_t vertexCount = meshopt_generateVertexRemap(remap.Data(), 0, indexCount, triangleVertices.Data(), indexCount, sizeof(BlitML::Vertex));

        BlitCL::DynamicArray<uint32_t> indices(indexCount);
        BlitCL::DynamicArray<BlitML::Vertex> vertices(vertexCount);

        meshopt_remapVertexBuffer(vertices.Data(), triangleVertices.Data(), indexCount, sizeof(BlitML::Vertex), remap.Data());
		meshopt_remapIndexBuffer(indices.Data(), 0, indexCount, remap.Data());

        // This is an algorithm from Arseny Kapoulkine that improves the way vertices are distributed for a mesh
        meshopt_optimizeVertexCache(indices.Data(), indices.Data(), indexCount, vertexCount);
	    meshopt_optimizeVertexFetch(vertices.Data(), indices.Data(), indexCount, vertices.Data(), 
        vertexCount, sizeof(BlitML::Vertex));

        PrimitiveSurface newSurface;
        newSurface.vertexOffset = static_cast<uint32_t>(resources.vertices.GetSize());
        newSurface.pMaterial = resources.materialTable.Get("loaded_material", &resources.materials[0]);

        //resources.indices.AddBlockAtBack(indices.Data(), indices.GetSize());
        resources.vertices.AddBlockAtBack(vertices.Data(), vertices.GetSize());

        BlitCL::DynamicArray<uint32_t> lodIndices(indices);
        while(newSurface.lodCount < BLIT_MAX_MESH_LOD)
        {
            // Get current element in the LOD array and increment the count
            MeshLod& lod = newSurface.meshLod[newSurface.lodCount++];

            lod.firstIndex = static_cast<uint32_t>(resources.indices.GetSize());
            lod.indexCount = static_cast<uint32_t>(lodIndices.GetSize());

            resources.indices.AddBlockAtBack(lodIndices.Data(), lodIndices.GetSize());

            if(newSurface.lodCount < BLIT_MAX_MESH_LOD)
            {
                size_t nextIndicesTarget = static_cast<size_t>(static_cast<double>(lodIndices.GetSize()) * 0.75);
                float nextError = 0.f;// Placeholder to fill the last parameter in the below function
                size_t nextIndices = meshopt_simplify(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), &vertices[0].position.x, 
                vertexCount, sizeof(BlitML::Vertex), nextIndicesTarget, 1e-4f, 0, &nextError);
                // If the next lod size surpasses the previous than this function has failed
                BLIT_ASSERT(nextIndices <= lodIndices.GetSize())

                // Reached the error bounds
                if(nextIndices == lodIndices.GetSize())
                    break;

                lodIndices.Downsize(nextIndices);
                // Optimize the indices cache
                meshopt_optimizeVertexCache(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), vertexCount);
            }
        }

        if(buildMeshlets)
        {
            BlitML::Meshlet meshlet = {};

            BlitCL::DynamicArray<uint8_t> meshletVertices(vertices.GetSize());
            meshletVertices.Fill(UINT8_MAX);

            newSurface.firstMeshlet = resources.meshlets.GetSize();

            for (size_t i = 0; i < indices.GetSize(); i += 3)
            {
                uint32_t vtxIndexA = indices[i + 0];
                uint32_t vtxIndexB = indices[i + 1];
                uint32_t vtxIndexC = indices[i + 2];

                uint8_t& vtxA = meshletVertices[vtxIndexA];
                uint8_t& vtxB = meshletVertices[vtxIndexB];
                uint8_t& vtxC = meshletVertices[vtxIndexC];

                // If the current meshlet's vertex count + the vertices that are going to be added next is over the meshlet vertex limit, 
                // It gets added to the dynamic array and a new entry is created
                if (meshlet.vertexCount + (vtxA == UINT8_MAX) + (vtxB == UINT8_MAX) + (vtxC == UINT8_MAX) > 64 || meshlet.triangleCount >= 126)
		        {
                    newSurface.meshletCount++;
			        resources.meshlets.PushBack(meshlet);

			        for (size_t j = 0; j < meshlet.vertexCount; ++j)
				        meshletVertices[meshlet.vertices[j]] = UINT8_MAX;
			        
                    meshlet = {};
		        }

                if (vtxA == UINT8_MAX)
		        {
		        	vtxA = meshlet.vertexCount;
		        	meshlet.vertices[meshlet.vertexCount++] = vtxIndexA;
		        }
		        if (vtxB == UINT8_MAX)
		        {
		        	vtxB = meshlet.vertexCount;
		        	meshlet.vertices[meshlet.vertexCount++] = vtxIndexB;
		        }
		        if (vtxC == UINT8_MAX)
		        {
		        	vtxC = meshlet.vertexCount;
		        	meshlet.vertices[meshlet.vertexCount++] = vtxIndexC;
		        }

                meshlet.indices[meshlet.triangleCount * 3 + 0] = vtxA;
		        meshlet.indices[meshlet.triangleCount * 3 + 1] = vtxB;
		        meshlet.indices[meshlet.triangleCount * 3 + 2] = vtxC;
                meshlet.triangleCount++;
            }
            if(meshlet.triangleCount)
            {
                resources.meshlets.PushBack(meshlet);
                newSurface.meshletCount++;
            }
        }

        BlitML::vec3 center(0.f);
        for(size_t i = 0; i < vertices.GetSize(); ++i)
        {
            center = center + vertices[i].position;// I have not overloaded the += operator
        }

        center = center / static_cast<float>(vertices.GetSize());// Again, I've only overloaded the regular operators

        float radius = 0;

        for(size_t i = 0; i < vertices.GetSize(); ++i)
        {
            BlitML::vec3 pos = vertices[i].position;
            radius = BlitML::Max(radius, BlitML::Distance(center, BlitML::vec3(pos.x, pos.y, pos.z)));
        }

        newSurface.center = center;
        newSurface.radius = radius;

        // Add the surface to the current mesh and increment the mesh index, so that the next time another mesh is processed
        resources.meshes[resources.currentMeshIndex].surfaces.PushBack(newSurface);
        ++resources.currentMeshIndex;

        return 1;
    }



    void LoadDefaultData(EngineResources& resources)
    {
        LoadMeshFromObj(resources, "Assets/Meshes/bunny.obj", 1);
        LoadMeshFromObj(resources, "Assets/Meshes/well.obj", 1);
    }
}