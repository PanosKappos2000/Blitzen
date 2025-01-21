#include "blitRenderingResources.h"
#include "blitRenderer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "VendorCode/stb_image.h"

#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"
#include "objparser.h"// Using some help from Arseny Kapoulkine
#include "Meshoptimizer/meshoptimizer.h"
#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"

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

    uint8_t LoadRenderingResourceSystem(RenderingResources* pResources)
    {
        pResources->textureTable.SetCapacity(BLIT_MAX_TEXTURE_COUNT);
        pResources->materialTable.SetCapacity(BLIT_MAX_MATERIAL_COUNT);

        return 1;
    }



    /*---------------------------------
        Texture specific functions
    ----------------------------------*/

    uint8_t LoadTextureFromFile(RenderingResources* pResources, const char* filename, const char* texName, 
    void* pVulkan, void* pDirectx12)
    {
        TextureStats& current = pResources->textures[pResources->currentTextureIndex];

        current.pTextureData = stbi_load(filename, &(current.textureWidth), &(current.textureHeight), &(current.textureChannels), 4);

        uint8_t load = current.pTextureData != nullptr;

        // Go to the next element in the container, only if the texture was loaded successfully
        if(load)
        {
            current.textureTag = static_cast<uint32_t>(pResources->currentTextureIndex);
            pResources->textureTable.Set(texName, &current);
            pResources->currentTextureIndex++;
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

    void LoadTestTextures(RenderingResources* pResources, void* pVulkan, void* pDx12)
    {
        // Default texture at index 0
        uint32_t blitTexCol = glm::packUnorm4x8(glm::vec4(0.3, 0, 0.6, 1));
        uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	    uint32_t pixels[16 * 16]; 
	    for (int x = 0; x < 16; x++) 
        {
	    	for (int y = 0; y < 16; y++) 
            {
	    		pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : blitTexCol;
	    	}
	    }
        pResources->textures[0].pTextureData = reinterpret_cast<uint8_t*>(pixels);
        pResources->textures[0].textureHeight = 1;
        pResources->textures[0].textureWidth = 1;
        pResources->textures[0].textureChannels = 4;
        pResources->textureTable.Set(BLIT_DEFAULT_TEXTURE_NAME, &(pResources->textures[0]));

        // This is hardcoded now
        LoadTextureFromFile(pResources, "Assets/Textures/cobblestone.png", "loaded_texture", pVulkan, nullptr);
        LoadTextureFromFile(pResources, "Assets/Textures/texture.jpg", "loaded_texture2", pVulkan, nullptr);
        LoadTextureFromFile(pResources, "Assets/Textures/cobblestone_SPEC.jpg", "spec_texture", pVulkan, nullptr);
    }



    void DefineMaterial(RenderingResources* pResources, BlitML::vec4& diffuseColor, float shininess, const char* diffuseMapName, 
    const char* specularMapName, const char* materialName)
    {
        Material& current = pResources->materials[pResources->currentMaterialIndex];

        current.diffuseColor = diffuseColor;
        current.shininess = shininess;

        current.diffuseTextureTag = pResources->textureTable.Get(diffuseMapName, &pResources->textures[0])->textureTag;
        current.specularTextureTag = pResources->textureTable.Get(specularMapName, &pResources->textures[0])->textureTag;

        current.materialId = static_cast<uint32_t>(pResources->currentMaterialIndex);

        pResources->materialTable.Set(materialName, &current);
        pResources->currentMaterialIndex++;
    }

    void LoadTestMaterials(RenderingResources* pResources, void* pVulkan, void* pDx12)
    {
        // Manually load a default material at index 0
        pResources->materials[0].diffuseColor = BlitML::vec4(1.f);
        pResources->materials[0].diffuseTextureTag = pResources->textureTable.Get(BLIT_DEFAULT_TEXTURE_NAME, &pResources->textures[0])->textureTag;
        pResources->materials[0].specularTextureTag = pResources->textureTable.Get(BLIT_DEFAULT_TEXTURE_NAME, &pResources->textures[0])->textureTag;
        pResources->materials[0].materialId = 0;
        pResources->materialTable.Set(BLIT_DEFAULT_MATERIAL_NAME, &(pResources->materials[0]));

        // Test code
        BlitML::vec4 color1(0.1f);
        BlitML::vec4 color2(0.2f);
        DefineMaterial(pResources, color1, 65.f, "loaded_texture", "spec_texture", "loaded_material");
        DefineMaterial(pResources, color2, 65.f, "loaded_texture2", "unknown", "loaded_material2");
    }
    



    uint8_t LoadMeshFromObj(RenderingResources* pResources, const char* filename, uint8_t buildMeshlets /*= 0*/)
    {
        // The function should return if the engine will go over the max allowed mesh assets
        if(pResources->currentMeshIndex > BLIT_MAX_MESH_COUNT)
        {
            BLIT_ERROR("Max mesh count: ( %i ) reached!", BLIT_MAX_MESH_COUNT)
            BLIT_INFO("If more objects are needed, increase the BLIT_MAX_MESH_COUNT macro before starting the loop")
            return 0;
        }

        // Get the current mesh and give it the size surface array as its first surface index
        Mesh& currentMesh = pResources->meshes[pResources->currentMeshIndex];
        currentMesh.firstSurface = static_cast<uint32_t>(pResources->surfaces.GetSize());

        ObjFile file;
        if(!objParseFile(file, filename))
            return 0;

        size_t indexCount = file.f_size / 3;

        /*!
            The below code can load a single mesh but it breaks if I try to load more than one
        */

        BlitCL::DynamicArray<Vertex> triangleVertices(indexCount);

        for(size_t i = 0; i < indexCount; ++i)
        {
            Vertex& vtx = triangleVertices[i];

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
		size_t vertexCount = meshopt_generateVertexRemap(remap.Data(), 0, indexCount, triangleVertices.Data(), indexCount, sizeof(Vertex));

        BlitCL::DynamicArray<uint32_t> indices(indexCount);
        BlitCL::DynamicArray<Vertex> vertices(vertexCount);

        meshopt_remapVertexBuffer(vertices.Data(), triangleVertices.Data(), indexCount, sizeof(Vertex), remap.Data());
		meshopt_remapIndexBuffer(indices.Data(), 0, indexCount, remap.Data());

        LoadSurface(pResources, vertices, indices, buildMeshlets);

        currentMesh.surfaceCount++;// Increment the surface count
        ++(pResources->currentMeshIndex);// Increment the mesh index

        return 1;
    }

    // The code for this function is taken from Arseny's niagara streams. It uses his meshoptimizer library which I am not that familiar with
    size_t LoadMeshlet(RenderingResources* pResources, BlitCL::DynamicArray<Vertex>& vertices, 
    BlitCL::DynamicArray<uint32_t>& indices)
    {
        const size_t maxVertices = 64;
        const size_t maxTriangles = 124;
        const float coneWeight = 0.25f;

        BlitCL::DynamicArray<meshopt_Meshlet> akMeshlets(meshopt_buildMeshletsBound(indices.GetSize(), maxVertices, maxTriangles));
        BlitCL::DynamicArray<unsigned int> meshletVertices(akMeshlets.GetSize() * maxVertices);
        BlitCL::DynamicArray<unsigned char> meshletTriangles(akMeshlets.GetSize() * maxTriangles * 3);

        akMeshlets.Downsize(meshopt_buildMeshlets(akMeshlets.Data(), meshletVertices.Data(), meshletTriangles.Data(), indices.Data(), indices.GetSize(), 
        &vertices[0].position.x, vertices.GetSize(), sizeof(Vertex), maxVertices, maxTriangles, coneWeight));

        
	    // note: I could append meshletVertices & meshletTriangles buffers more or less directly with small changes in Meshlet struct, 
        // but for now keep the GPU side layout flexible and separate
        for(size_t i = 0; i < akMeshlets.GetSize(); ++i)
        {
            meshopt_Meshlet& meshlet = akMeshlets[i];

            meshopt_optimizeMeshlet(&meshletVertices[meshlet.vertex_offset], &meshletTriangles[meshlet.triangle_offset], 
            meshlet.triangle_count, meshlet.vertex_count);

            size_t dataOffset = pResources->meshletData.GetSize();
            for(unsigned int i = 0; i < meshlet.vertex_count; ++i)
            {
                pResources->meshletData.PushBack(meshletVertices[meshlet.vertex_offset + i]);
            }

            const unsigned int* indexGroups = reinterpret_cast<const unsigned int*>(&meshletTriangles[0] + meshlet.triangle_offset);
            unsigned int indexGroupCount = (meshlet.triangle_count * 3 + 3);

            for(unsigned int i = 0; i < indexGroupCount; ++i)
            {
                pResources->meshletData.PushBack(uint32_t(indexGroups[size_t(i)]));
            }

            meshopt_Bounds bounds = meshopt_computeMeshletBounds(&meshletVertices[meshlet.vertex_offset], 
            &meshletTriangles[meshlet.triangle_offset], meshlet.triangle_count, &vertices[0].position.x, vertices.GetSize(), sizeof(Vertex));

            Meshlet m = {};
            m.dataOffset = static_cast<uint32_t>(dataOffset);
            m.triangleCount = meshlet.triangle_count;
            m.vertexCount = meshlet.vertex_count;

            m.center = BlitML::vec3(bounds.center[0], bounds.center[1], bounds.center[2]);
            m.radius = bounds.radius;
            m.cone_axis[0] = bounds.cone_axis_s8[0];
		    m.cone_axis[1] = bounds.cone_axis_s8[1];
		    m.cone_axis[2] = bounds.cone_axis_s8[2];
		    m.cone_cutoff = bounds.cone_cutoff_s8; 

            pResources->meshlets.PushBack(m);
        }

        return akMeshlets.GetSize();
    }

    void LoadSurface(RenderingResources* pResources, BlitCL::DynamicArray<Vertex>& vertices, BlitCL::DynamicArray<uint32_t>& indices, 
    uint8_t buildMeshlets)
    {
        // This is an algorithm from Arseny Kapoulkine that improves the way vertices are distributed for a mesh
        meshopt_optimizeVertexCache(indices.Data(), indices.Data(), indices.GetSize(), vertices.GetSize());
	    meshopt_optimizeVertexFetch(vertices.Data(), indices.Data(), indices.GetSize(), vertices.Data(), 
        vertices.GetSize(), sizeof(Vertex));

        // Create the new surface that will be added and initialize its vertex offset
        PrimitiveSurface newSurface;
        newSurface.vertexOffset = static_cast<uint32_t>(pResources->vertices.GetSize());
        /* newSurface.materialId = pResources->materialTable.Get("loaded_material", &pResources->materials[0])->materialId;
        Harcoded material that is not being used, probably will remove this */

        // Since the vertices will be global for all shaders and objects, new elements will be added to the one vertex array
        pResources->vertices.AddBlockAtBack(vertices.Data(), vertices.GetSize());

        BlitCL::DynamicArray<uint32_t> lodIndices(indices);
        while(newSurface.lodCount < BLIT_MAX_MESH_LOD)
        {
            // Get current element in the LOD array and increment the count
            MeshLod& lod = newSurface.meshLod[newSurface.lodCount++];

            lod.firstIndex = static_cast<uint32_t>(pResources->indices.GetSize());
            lod.indexCount = static_cast<uint32_t>(lodIndices.GetSize());

            lod.firstMeshlet = static_cast<uint32_t>(pResources->meshlets.GetSize());
            lod.meshletCount = buildMeshlets ? static_cast<uint32_t>(LoadMeshlet(pResources, vertices, indices)) : 0;

            pResources->indices.AddBlockAtBack(lodIndices.Data(), lodIndices.GetSize());

            if(newSurface.lodCount < BLIT_MAX_MESH_LOD)
            {
                size_t nextIndicesTarget = static_cast<size_t>((double(lodIndices.GetSize()) * 0.65) / 3) * 3;
                float nextError = 0.f;// Placeholder to fill the last parameter in the below function
                size_t nextIndices = meshopt_simplify(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), &vertices[0].position.x, 
                vertices.GetSize(), sizeof(Vertex), nextIndicesTarget, 1e-1f, 0, &nextError);
                // If the next lod size surpasses the previous than this function has failed
                BLIT_ASSERT(nextIndices <= lodIndices.GetSize())

                // Reached the error bounds
                if(nextIndices == lodIndices.GetSize())
                    break;

                lodIndices.Downsize(nextIndices);
                // Optimize the indices cache
                meshopt_optimizeVertexCache(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), vertices.GetSize());
            }
        }

        // Calculate the center of the bounding sphere for the surface
        BlitML::vec3 center(0.f);
        for(size_t i = 0; i < vertices.GetSize(); ++i)
        {
            center = center + vertices[i].position;// I have not overloaded the += operator
        }
        center = center / static_cast<float>(vertices.GetSize());// Again, I've only overloaded the regular operators

        // After calculating the center, calculate the radius of the bounding sphere
        float radius = 0;
        for(size_t i = 0; i < vertices.GetSize(); ++i)
        {
            BlitML::vec3 pos = vertices[i].position;
            radius = BlitML::Max(radius, BlitML::Distance(center, BlitML::vec3(pos.x, pos.y, pos.z)));
        }

        // Pass the center and radius to the surface, so that bounding sphere data is available for frustum culling
        newSurface.center = center;
        newSurface.radius = radius;

        pResources->surfaces.PushBack(newSurface);
    }



    void LoadTestGeometry(RenderingResources* pResources)
    {
        LoadMeshFromObj(pResources, "Assets/Meshes/dragon.obj", 1);
        LoadMeshFromObj(pResources, "Assets/Meshes/kitten.obj", 1);
        LoadMeshFromObj(pResources, "Assets/Meshes/bunny.obj", 1);
        LoadMeshFromObj(pResources, "Assets/Meshes/FinalBaseMesh.obj", 1);
    }


    void CreateTestGameObjects(RenderingResources* pResources, uint32_t drawCount)
    {
        pResources->objectCount = drawCount;// Normally the draw count differs from the game object count, but the engine is really simple at the moment
        pResources->transforms.Resize(pResources->objectCount);// Every object has a different transform
        // Hardcode a large amount of male model mesh
        for(size_t i = 0; i < pResources->objectCount / 10; ++i)
        {
            BlitzenEngine::MeshTransform& transform = pResources->transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1'500 - 50,//x 
            (float(rand()) / RAND_MAX) * 1'500 - 50,//y
            (float(rand()) / RAND_MAX) * 1'500 - 50);//z
            transform.pos = translation;
            transform.scale = 0.1f;

            // Loading random orientation. Normally you would get this from the game object
            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
            (float(rand()) / RAND_MAX) * 2 - 1, // y
            (float(rand()) / RAND_MAX) * 2 - 1); // z
		    float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            transform.orientation = orientation;

            GameObject& currentObject = pResources->objects[i];

            currentObject.meshIndex = 3;// Hardcode the bunny mesh for each object in this loop
            currentObject.transformIndex = static_cast<uint32_t>(i);// Transform index is the same as the object index
        }
        // Hardcode a large amount of objects with the high polygon kitten mesh and random transforms
        for (size_t i = pResources->objectCount / 10; i < pResources->objectCount / 8; ++i)
        {
            BlitzenEngine::MeshTransform& transform = pResources->transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1'500 - 50,//x 
            (float(rand()) / RAND_MAX) * 1'500 - 50,//y
            (float(rand()) / RAND_MAX) * 1'500 - 50);//z
            transform.pos = translation;
            transform.scale = 1.f;

            // Loading random orientation. Normally you would get this from the game object
            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
            (float(rand()) / RAND_MAX) * 2 - 1, // y
            (float(rand()) / RAND_MAX) * 2 - 1); // z
		    float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            transform.orientation = orientation;

            GameObject& currentObject = pResources->objects[i];

            currentObject.meshIndex = 1;// Hardcode the kitten mesh for each object in this loop
            currentObject.transformIndex = static_cast<uint32_t>(i);// Transform index is the same as the object index
        }
        // Hardcode a large amount of stanford dragons
        for (size_t i = pResources->objectCount / 8; i < pResources->objectCount / 6; ++i)
        {
            BlitzenEngine::MeshTransform& transform = pResources->transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1'500 - 50,//x 
                (float(rand()) / RAND_MAX) * 1'500 - 50,//y
                (float(rand()) / RAND_MAX) * 1'500 - 50);//z
            transform.pos = translation;
            transform.scale = 0.1f;

            // Loading random orientation. Normally you would get this from the game object
            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
                (float(rand()) / RAND_MAX) * 2 - 1, // y
                (float(rand()) / RAND_MAX) * 2 - 1); // z
            float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            transform.orientation = orientation;

            GameObject& currentObject = pResources->objects[i];

            currentObject.meshIndex = 0;// Hardcode the kitten mesh for each object in this loop
            currentObject.transformIndex = static_cast<uint32_t>(i);// Transform index is the same as the object index
        }
        // Hardcode a large amount of standford bunnies
        for (size_t i = pResources->objectCount / 6; i < pResources->objectCount; ++i)
        {
            BlitzenEngine::MeshTransform& transform = pResources->transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1'500 - 50,//x 
                (float(rand()) / RAND_MAX) * 1'500 - 50,//y
                (float(rand()) / RAND_MAX) * 1'500 - 50);//z
            transform.pos = translation;
            transform.scale = 5.f;

            // Loading random orientation. Normally you would get this from the game object
            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
                (float(rand()) / RAND_MAX) * 2 - 1, // y
                (float(rand()) / RAND_MAX) * 2 - 1); // z
            float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            transform.orientation = orientation;

            GameObject& currentObject = pResources->objects[i];

            currentObject.meshIndex = 2;// Hardcode the kitten mesh for each object in this loop
            currentObject.transformIndex = static_cast<uint32_t>(i);// Transform index is the same as the object index
        }

        // Create all the render objects by getting the data from the game objects
        for(size_t i = 0; i < pResources->objectCount; ++i)
        {
            // Get the mesh used by the current game object
            BlitzenEngine::Mesh& currentMesh = pResources->meshes[pResources->objects[i].meshIndex];

            for(size_t j = 0; j < currentMesh.surfaceCount; ++j)
            {
                RenderObject& currentObject = pResources->renders[pResources->renderObjectCount];

                // Get the surface Id for this render object by adding the current index to the first surface of the mesh
                currentObject.surfaceId = currentMesh.firstSurface + static_cast<uint32_t>(j);
                
                currentObject.transformId = pResources->objects[i].transformIndex;

                pResources->renderObjectCount++;
            }
        }
    }

    // Calls some test functions to load a scene that tests the renderer's geometry rendering
    void LoadGeometryStressTest(RenderingResources* pResources, uint32_t drawCount, void* pVulkan, void* pDx12)
    {
        LoadTestTextures(pResources, pVulkan, nullptr);
        LoadTestMaterials(pResources, pVulkan, nullptr);
        LoadTestGeometry(pResources);
        CreateTestGameObjects(pResources, drawCount);
    }

    uint8_t LoadGltfScene(RenderingResources* pResources, const char* path, uint8_t buildMeshlets /*=1*/)
    {
        cgltf_options options = {};
	    cgltf_data* data = nullptr;
	    cgltf_result res = cgltf_parse_file(&options, path, &data);
	    if (res != cgltf_result_success)
        {
            BLIT_WARN("Failed to load gltf file: %s", path)
		    return 0;
        }

        res = cgltf_load_buffers(&options, data, path);
	    if (res != cgltf_result_success)
	    {
		    cgltf_free(data);
		    return false;
	    }

	    res = cgltf_validate(data);
	    if (res != cgltf_result_success)
	    {
		    cgltf_free(data);
		    return false;
	    }

        // The surface indices is a list of the first surface of each mesh. Used to create the render object struct
        BlitCL::DynamicArray<uint32_t> surfaceIndices;

        // Defining a lambda here for finding the accessor since it will probably not be used outside of this
        auto findAccessor = [](const cgltf_primitive* prim,  cgltf_attribute_type type, cgltf_int index = 0){
            for (size_t i = 0; i < prim->attributes_count; ++i)
	        {
		        const cgltf_attribute& attr = prim->attributes[i];
		        if (attr.type == type && attr.index == index)
			        return attr.data;
	        }

            // This might seem redundant but compilation fails if I do not do this
            cgltf_accessor* scratch = nullptr;
	        return scratch;
        };

        for (size_t i = 0; i < data->meshes_count; ++i)
	    {
            // Get the current mesh
		    const cgltf_mesh& mesh = data->meshes[i];

            // Find the first surface of the current mesh. 
            // It is important for the mesh structu and to save the data for later to create the render objects
            uint32_t firstSurface = pResources->surfaces.GetSize();

            // Tell the new mesh the surfaces that it should draw
            pResources->meshes[pResources->currentMeshIndex].firstSurface = firstSurface;
            pResources->meshes[pResources->currentMeshIndex].surfaceCount = mesh.primitives_count;
            pResources->currentMeshIndex++;

		    surfaceIndices.PushBack(firstSurface);

            for(size_t j = 0; j < mesh.primitives_count; ++j)
            {
                const cgltf_primitive& prim = mesh.primitives[j];
			    BLIT_ASSERT(prim.type == cgltf_primitive_type_triangles);
			    BLIT_ASSERT(prim.indices);

			    size_t vertexCount = prim.attributes[0].data->count;

			    BlitCL::DynamicArray<Vertex> vertices(vertexCount);
			    BlitCL::DynamicArray<float> scratch(vertexCount * 4);

			    if (const cgltf_accessor* pos = findAccessor(&prim, cgltf_attribute_type_position))
			    {
				    assert(cgltf_num_components(pos->type) == 3);
				    cgltf_accessor_unpack_floats(pos, scratch.Data(), vertexCount * 3);
				    for (size_t j = 0; j < vertexCount; ++j)
				    {
					    vertices[j].position = BlitML::vec3(scratch[j * 3 + 0], scratch[j * 3 + 1], scratch[j * 3 + 2]);
				    }
			    }

			    if (const cgltf_accessor* nrm = findAccessor(&prim, cgltf_attribute_type_normal))
			    {
			    	assert(cgltf_num_components(nrm->type) == 3);
			    	cgltf_accessor_unpack_floats(nrm, scratch.Data(), vertexCount * 3);
			    	for (size_t j = 0; j < vertexCount; ++j)
			    	{
			    		vertices[j].normal = BlitML::vec3(scratch[j * 3 + 0] * 127.f + 127.5f, 
                        scratch[j * 3 + 1] * 127.f + 127.5f, scratch[j * 3 + 2] * 127.f + 127.5f);
			    	}
			    }

			    if (const cgltf_accessor* tex = findAccessor(&prim, cgltf_attribute_type_texcoord))
			    {
				    assert(cgltf_num_components(tex->type) == 2);
				    cgltf_accessor_unpack_floats(tex, scratch.Data(), vertexCount * 2);
				    for (size_t j = 0; j < vertexCount; ++j)
				    {
					    vertices[j].uvX = meshopt_quantizeHalf(scratch[j * 2 + 0]);
					    vertices[j].uvY = meshopt_quantizeHalf(scratch[j * 2 + 1]);
				    }
			    }

                BlitCL::DynamicArray<uint32_t> indices(prim.indices->count);
			    cgltf_accessor_unpack_indices(prim.indices, indices.Data(), 4, indices.GetSize());

                LoadSurface(pResources, vertices, indices, buildMeshlets);

                // Adding this surface to the render objects, there will be a render object for each surface 
                // No game objects will be used for a gltf scene as it is a scene and not a character really
                pResources->renders[pResources->renderObjectCount].surfaceId = pResources->surfaces.Back().surfaceId;
            }
        }

        for (size_t i = 0; i < data->nodes_count; ++i)
	    {
		    const cgltf_node* node = &data->nodes[i];
		    if (node->mesh)
		    {
			    float matrix[16];
			    cgltf_node_transform_world(node, matrix);

			    float translation[3];
			    float rotation[4];
			    float scale[3];

			    BlitML::decomposeTransform(translation, rotation, scale, matrix);
                MeshTransform transform;
				transform.pos = BlitML::vec3(translation[0], translation[1], translation[2]);
				transform.scale = BlitML::Max(scale[0], BlitML::Max(scale[1], scale[2]));
				transform.orientation = BlitML::quat(rotation[0], rotation[1], rotation[2], rotation[3]);

			    // TODO: better warnings for non-uniform or negative scale

                // Hold the offset of the first surface of the mesh and the transform id to give to the render objects
			    uint32_t surfaceOffset = surfaceIndices[cgltf_mesh_index(data, node->mesh)];
                uint32_t transformId = pResources->transforms.GetSize();

			    for (unsigned int j = 0; j < node->mesh->primitives_count; ++j)
			    {
                    RenderObject& current = pResources->renders[pResources->renderObjectCount];
                    current.surfaceId = surfaceOffset + j;
                    current.transformId = transformId;
                    
                    // Increment the render object count
                    pResources->renderObjectCount++;
			    }

                pResources->transforms.PushBack(transform);
		    }
	    }
	    cgltf_free(data);

        return 1;
    }
}