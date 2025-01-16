#include "blitRenderingResources.h"
#include "BlitzenVulkan/vulkanRenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "VendorCode/stb_image.h"

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "objparser.h"// Using some help from Arseny Kapoulkine
#include "Meshoptimizer/meshoptimizer.h"

// My math library seems to be fine now but I am keeping this to compare values when needed
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

    uint8_t LoadTextureFromFile(EngineResources& resources, const char* filename, const char* texName, 
    void* pVulkan, void* pDirectx12)
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
            current.textureTag = BLIT_DEFAULT_TEXTURE_COUNT - 1;
        }

        // If the vulkan renderer was given to the function and the data from the file was successfully loaded, the texture is passed to the renderer
        if(pVulkan && load)
        {
            BlitzenVulkan::VulkanRenderer* pRenderer = reinterpret_cast<BlitzenVulkan::VulkanRenderer*>(pVulkan);

            pRenderer->UploadTexture(current);
        }

        // Return 1 if the data from the file was loaded and 0 otherwise
        return load;
    }



    /*----------------------------------
        Material specific functions
    -----------------------------------*/

    void DefineMaterial(EngineResources& resources, BlitML::vec4& diffuseColor, float shininess, const char* diffuseMapName, 
    const char* specularMapName, const char* materialName)
    {
        Material& current = resources.materials[resources.currentMaterialIndex];

        current.diffuseColor = diffuseColor;
        current.shininess = shininess;

        current.diffuseTextureTag = resources.textureTable.Get(diffuseMapName, &resources.textures[0])->textureTag;
        current.specularTextureTag = resources.textureTable.Get(specularMapName, &resources.textures[0])->textureTag;

        current.materialId = static_cast<uint32_t>(resources.currentMaterialIndex);

        resources.materialTable.Set(materialName, &current);
        resources.currentMaterialIndex++;
    }



    uint8_t LoadMeshFromObj(EngineResources& resources, const char* filename, uint8_t buildMeshlets /*= 0*/)
    {
        // The function should return if the engine will go over the max allowed mesh assets
        if(resources.currentMeshIndex > BLIT_MAX_MESH_COUNT)
        {
            BLIT_ERROR("Max mesh count: ( %i ) reached!", BLIT_MAX_MESH_COUNT)
            BLIT_INFO("If more objects are needed, increase the BLIT_MAX_MESH_COUNT macro before starting the loop")
            return 0;
        }

        // Get the current mesh and give it the size surface array as its first surface index
        Mesh& currentMesh = resources.meshes[resources.currentMeshIndex];
        currentMesh.firstSurface = static_cast<uint32_t>(resources.surfaces.GetSize());

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
        newSurface.materialId = resources.materialTable.Get("loaded_material", &resources.materials[0])->materialId;

        //resources.indices.AddBlockAtBack(indices.Data(), indices.GetSize());
        resources.vertices.AddBlockAtBack(vertices.Data(), vertices.GetSize());

        BlitCL::DynamicArray<uint32_t> lodIndices(indices);
        while(newSurface.lodCount < BLIT_MAX_MESH_LOD)
        {
            // Get current element in the LOD array and increment the count
            MeshLod& lod = newSurface.meshLod[newSurface.lodCount++];

            lod.firstIndex = static_cast<uint32_t>(resources.indices.GetSize());
            lod.indexCount = static_cast<uint32_t>(lodIndices.GetSize());

            lod.firstMeshlet = static_cast<uint32_t>(resources.meshlets.GetSize());
            lod.meshletCount = buildMeshlets ? static_cast<uint32_t>(LoadMeshlet(resources, vertices, indices)) : 0;

            resources.indices.AddBlockAtBack(lodIndices.Data(), lodIndices.GetSize());

            if(newSurface.lodCount < BLIT_MAX_MESH_LOD)
            {
                size_t nextIndicesTarget = static_cast<size_t>((double(lodIndices.GetSize()) * 0.65) / 3) * 3;
                float nextError = 0.f;// Placeholder to fill the last parameter in the below function
                size_t nextIndices = meshopt_simplify(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), &vertices[0].position.x, 
                vertices.GetSize(), sizeof(BlitML::Vertex), nextIndicesTarget, 1e-1f, 0, &nextError);
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
        resources.surfaces.PushBack(newSurface);
        currentMesh.surfaceCount++;// Increment the surface count
        ++resources.currentMeshIndex;// Increment the mesh index

        return 1;
    }

    // The code for this function is taken from Arseny's niagara streams. It uses his meshoptimizer library which I am not that familiar with
    size_t LoadMeshlet(EngineResources& resources, BlitCL::DynamicArray<BlitML::Vertex>& vertices, 
    BlitCL::DynamicArray<uint32_t>& indices)
    {
        const size_t maxVertices = 64;
        const size_t maxTriangles = 124;
        const float coneWeight = 0.25f;

        BlitCL::DynamicArray<meshopt_Meshlet> akMeshlets(meshopt_buildMeshletsBound(indices.GetSize(), maxVertices, maxTriangles));
        BlitCL::DynamicArray<unsigned int> meshletVertices(akMeshlets.GetSize() * maxVertices);
        BlitCL::DynamicArray<unsigned char> meshletTriangles(akMeshlets.GetSize() * maxTriangles * 3);

        akMeshlets.Downsize(meshopt_buildMeshlets(akMeshlets.Data(), meshletVertices.Data(), meshletTriangles.Data(), indices.Data(), indices.GetSize(), 
        &vertices[0].position.x, vertices.GetSize(), sizeof(BlitML::Vertex), maxVertices, maxTriangles, coneWeight));

        
	    // note: I could append meshletVertices & meshletTriangles buffers more or less directly with small changes in Meshlet struct, 
        // but for now keep the GPU side layout flexible and separate
        for(size_t i = 0; i < akMeshlets.GetSize(); ++i)
        {
            meshopt_Meshlet& meshlet = akMeshlets[i];

            meshopt_optimizeMeshlet(&meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset], 
            meshlet.triangle_count, meshlet.vertex_count);

            size_t dataOffset = resources.meshletData.GetSize();
            for(unsigned int i = 0; i < meshlet.vertex_count; ++i)
            {
                resources.meshletData.PushBack(meshletVertices[meshlet.vertex_offset + i]);
            }

            const unsigned int* indexGroups = reinterpret_cast<const unsigned int*>(&meshletTriangles[0] + meshlet.triangle_offset);
            unsigned int indexGroupCount = (meshlet.triangle_count * 3 + 3);

            for(unsigned int i = 0; i < indexGroupCount; ++i)
            {
                resources.meshletData.PushBack(uint32_t(indexGroups[size_t(i)]));
            }

            meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshletVertices[meshlet.vertex_offset], 
            &meshletTriangles[meshlet.triangle_offset], meshlet.triangle_count, &vertices[0].position.x, vertices.GetSize(), sizeof(BlitML::Vertex));

            BlitML::Meshlet m = {};
            m.dataOffset = static_cast<uint32_t>(dataOffset);
            m.triangleCount = meshlet.triangle_count;
            m.vertexCount = meshlet.vertex_count;

            m.center = BlitML::vec3(bounds.center[0], bounds.center[1], bounds.center[2]);
            m.radius = bounds.radius;
            m.cone_axis[0] = bounds.cone_axis_s8[0];
		    m.cone_axis[1] = bounds.cone_axis_s8[1];
		    m.cone_axis[2] = bounds.cone_axis_s8[2];
		    m.cone_cutoff = bounds.cone_cutoff_s8; 

            resources.meshlets.PushBack(m);
        }

        return akMeshlets.GetSize();
    }



    void LoadDefaultData(EngineResources& resources)
    {
        LoadMeshFromObj(resources, "Assets/Meshes/dragon.obj", 1);
        LoadMeshFromObj(resources, "Assets/Meshes/kitten.obj", 1);
        LoadMeshFromObj(resources, "Assets/Meshes/bunny.obj", 1);
        LoadMeshFromObj(resources, "Assets/Meshes/FinalBaseMesh.obj", 1);
    }
}