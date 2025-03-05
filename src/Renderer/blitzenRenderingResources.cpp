#include "blitRenderingResources.h"
#include "blitRenderer.h"

// Single file .png and .jpeg image loader, to be used for textures
// https://github.com/nothings/stb
#define STB_IMAGE_IMPLEMENTATION
#include "VendorCode/stb_image.h"

// Used for loading .obj meshes
// https://github.com/thisistherk/fast_obj
#define FAST_OBJ_IMPLEMENTATION
#include "fast_obj.h"

// Algorithms for building meshlets, loading LODs, optimizing vertex caches etc.
// https://github.com/zeux/meshoptimizer
#include "Meshoptimizer/meshoptimizer.h"
#include "objparser.h"

// Single file gltf loading https://github.com/jkuhlmann/cgltf
#define CGLTF_IMPLEMENTATION
#include "Cgltf/cgltf.h"

// Not necessary since I have my own math library, but I use glm for random values in textures
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"

#include <string>

namespace BlitzenEngine
{
    // This will not be used outside of this translation unit, so it has not declaration
    static uint8_t LoadTextureFromFile(RenderingResources* pResources, const char* filename, const char* texName, 
    RendererPtrType pRenderer)
    {
        // Don't go over the texture limit, might want to throw a warning here
        if (pResources->textureCount >= ce_maxTextureCount)
        {
            BLIT_WARN("Max texture count: %i, has been reached", ce_maxTextureCount)
            BLIT_ERROR("Texture from file: %s, failed to load", filename)
            return 0;
        }
    
        TextureStats& texture = pResources->textures[pResources->textureCount];
        // TODO: Oboslete, create them inside the Upload texture function
        BlitzenEngine::DDS_HEADER header{};
        BlitzenEngine::DDS_HEADER_DXT10 header10{};
            
        // If texture upload to the renderer succeeds, the texture count is incremented and the function returns successfully
        if(pRenderer->UploadTexture(header, header10, texture.pTextureData, filename))
        {
            pResources->textureCount++;
            return 1;
        }
    
        BLIT_ERROR("Texture from file: %s, failed to load", filename)
        return 0;
    }

    uint8_t LoadRenderingResourceSystem(RenderingResources* pResources, void* rendererData)
    {
        auto pRenderer = reinterpret_cast<RendererPtrType>(rendererData);

        // Creates one default texture and one default material
        LoadTextureFromFile(pResources, "Assets/Textures/base_baseColor.dds", 
        "dds_texture_default", pRenderer);
        BlitML::vec4 color1(0.1f);
        DefineMaterial(pResources, color1, 65.f, "dds_texture_default", "unknown", "loaded_material");

        return 1;
    }

    void DefineMaterial(RenderingResources* pResources, BlitML::vec4& diffuseColor, float shininess, const char* diffuseMapName, 
    const char* specularMapName, const char* materialName)
    {
        Material& current = pResources->materials[pResources->materialCount];

        current.diffuseColor = diffuseColor;
        current.shininess = shininess;

        current.albedoTag = 0; //pResources->textureTable[diffuseMapName].textureTag;
        current.normalTag = 0; //pResources->textureTable[specularMapName].textureTag;
        current.specularTag = 0;
        current.emissiveTag = 0;

        current.materialId = static_cast<uint32_t>(pResources->materialCount);

        pResources->materialTable.Insert(materialName, current);
        pResources->materialCount++;
    }

    uint8_t LoadMeshFromObj(RenderingResources* pResources, const char* filename)
    {
        // The function should return if the engine will go over the max allowed mesh assets
        if(pResources->meshCount > ce_maxMeshCount)
        {
            BLIT_ERROR("Max mesh count: ( %i ) reached!", ce_maxMeshCount)
            BLIT_INFO("If more objects are needed, increase the BLIT_MAX_MESH_COUNT macro before starting the loop")
            return 0;
        }

        BLIT_INFO("Loading obj model form file: %s", filename)

        // Get the current mesh and give it the size surface array as its first surface index
        Mesh& currentMesh = pResources->meshes[pResources->meshCount];
        currentMesh.firstSurface = static_cast<uint32_t>(pResources->surfaces.GetSize());

        ObjFile file;
        if(!objParseFile(file, filename))
            return 0;

        size_t indexCount = file.f_size / 3;

        BlitCL::DynamicArray<Vertex> triangleVertices(indexCount);

        BLIT_INFO("Loading vertices and indices")

        for(size_t i = 0; i < indexCount; ++i)
        {
            Vertex& vtx = triangleVertices[i];

            int32_t vertexIndex = file.f[i * 3 + 0];
		    int32_t vertexTextureIndex = file.f[i * 3 + 1];
		    int32_t vertexNormalIndex = file.f[i * 3 + 2];

            vtx.position.x = file.v[vertexIndex * 3 + 0];
		    vtx.position.y = file.v[vertexIndex * 3 + 1];
		    vtx.position.z = file.v[vertexIndex * 3 + 2];

            // Load the normal and turn them to 8 bit integers
		    float normalX = vertexNormalIndex < 0 ? 0.f : file.vn[vertexNormalIndex * 3 + 0];
		    float normalY = vertexNormalIndex < 0 ? 0.f : file.vn[vertexNormalIndex * 3 + 1];
		    float normalZ = vertexNormalIndex < 0 ? 1.f : file.vn[vertexNormalIndex * 3 + 2];
            vtx.normalX = static_cast<uint8_t>(normalX * 127.f + 127.5f);
            vtx.normalY = static_cast<uint8_t>(normalY * 127.f + 127.5f);
            vtx.normalZ = static_cast<uint8_t>(normalZ * 127.f + 127.5f);

            vtx.tangentX = vtx.tangentY = vtx.tangentZ = 127;
            vtx.tangentW = 254;

		    vtx.uvX = meshopt_quantizeHalf(vertexTextureIndex < 0 ? 0.f : file.vt[vertexTextureIndex * 3 + 0]);
		    vtx.uvY = meshopt_quantizeHalf(vertexTextureIndex < 0 ? 0.f : file.vt[vertexTextureIndex * 3 + 1]);
        }

        BlitCL::DynamicArray<uint32_t> remap(indexCount);
		size_t vertexCount = meshopt_generateVertexRemap(remap.Data(), 0, indexCount, triangleVertices.Data(), indexCount, sizeof(Vertex));

        BlitCL::DynamicArray<uint32_t> indices(indexCount);
        BlitCL::DynamicArray<Vertex> vertices(vertexCount);

        meshopt_remapVertexBuffer(vertices.Data(), triangleVertices.Data(), indexCount, sizeof(Vertex), remap.Data());
		meshopt_remapIndexBuffer(indices.Data(), 0, indexCount, remap.Data());

        BLIT_INFO("Creating surface")
        LoadPrimitiveSurface(pResources, vertices, indices);

        currentMesh.surfaceCount++;// Increment the surface count
        ++(pResources->meshCount);// Increment the mesh count

        return 1;
    }

    // The code for this function is taken from Arseny's niagara streams. It uses his meshoptimizer library which I am not that familiar with
    size_t GenerateClusters(RenderingResources* pResources, BlitCL::DynamicArray<Vertex>& vertices, 
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

            unsigned int* indexGroups = reinterpret_cast<unsigned int*>(&meshletTriangles[0] + meshlet.triangle_offset);
            unsigned int indexGroupCount = (meshlet.triangle_count * 3 + 3);

            for(unsigned int i = 0; i < indexGroupCount; ++i)
            {
                pResources->meshletData.PushBack(indexGroups[size_t(i)]);
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

    void LoadPrimitiveSurface(RenderingResources* pResources, 
    BlitCL::DynamicArray<Vertex>& vertices, 
    BlitCL::DynamicArray<uint32_t>& indices)
    {
        // This is an algorithm from Arseny Kapoulkine that improves the way vertices are distributed for a primitive
        meshopt_optimizeVertexCache(indices.Data(), indices.Data(), indices.GetSize(), vertices.GetSize());
	    meshopt_optimizeVertexFetch(vertices.Data(), indices.Data(), indices.GetSize(), vertices.Data(), 
        vertices.GetSize(), sizeof(Vertex));

        // Create the new surface that will be added and initialize its vertex offset
        PrimitiveSurface newSurface;
        newSurface.vertexOffset = static_cast<uint32_t>(pResources->vertices.GetSize());

        // Since the vertices will be global for all shaders and objects, new elements will be added to the one vertex array
        pResources->vertices.AppendArray(vertices);

        // Create the normal array to be used with the meshoptimizer function for lod generation
        BlitCL::DynamicArray<BlitML::vec3> normals(vertices.GetSize());
	    for (size_t i = 0; i < vertices.GetSize(); ++i)
	    {
		    Vertex& v = vertices[i];
		    normals[i] = BlitML::vec3(v.normalX / 127.f - 1.f, v.normalY / 127.f - 1.f, v.normalZ / 127.f - 1.f);
	    }

        // This will be used to take into accout surface dimensions when saving the lod error
        float lodScale = meshopt_simplifyScale(&vertices[0].position.x, vertices.GetSize(), sizeof(Vertex));

        // Lod error will be passed as a pointer every time the meshopt lod genration function is called, to save the next error
        float lodError = 0.f;

	    float normalWeights[3] = {1.f, 1.f, 1.f};

        // Pass the original loaded indices of the surface to the new lod indices
        BlitCL::DynamicArray<uint32_t> lodIndices(indices);

        while(newSurface.lodCount < ce_primitiveSurfaceMaxLODCount)
        {
            // Get current element in the LOD array and increment the count
            MeshLod& lod = newSurface.meshLod[newSurface.lodCount++];

            // Save the indices that will be used for the current lod level
            lod.firstIndex = static_cast<uint32_t>(pResources->indices.GetSize());
            lod.indexCount = static_cast<uint32_t>(lodIndices.GetSize());

            // Save the meshlets that will be used for the current lod level
            lod.firstMeshlet = static_cast<uint32_t>(pResources->meshlets.GetSize());
            lod.meshletCount = ce_buildClusters ? static_cast<uint32_t>(GenerateClusters(pResources, vertices, indices)) : 0;

            // Add the new indices that were loaded for this lod level to the global index buffer
            pResources->indices.AppendArray(lodIndices);

            // Save the current lod error
            lod.error = lodError * lodScale;

            if(newSurface.lodCount < ce_primitiveSurfaceMaxLODCount)
            {
                // Specify the next target index count (simplify by 65% each time)
                size_t nextIndicesTarget = static_cast<size_t>((double(lodIndices.GetSize()) * 0.65) / 3) * 3;

                const float maxError = 1e-1f;// Parameter for meshopt_simplifyWithAttributes

                // The next error will be saved here to check if the actual lod error should be updated
                float nextError = 0;

                // Generate the indices
                size_t nextIndicesSize = meshopt_simplifyWithAttributes(lodIndices.Data(), lodIndices.Data(), 
                lodIndices.GetSize(), &vertices[0].position.x, 
                vertices.GetSize(), sizeof(Vertex), &normals[0].x, sizeof(BlitML::vec3), 
                normalWeights, 3, nullptr, nextIndicesTarget, maxError, 0, &nextError);

                // If the next lod size surpasses the previous than this function has failed
                BLIT_ASSERT(nextIndicesSize <= lodIndices.GetSize())

                // Reached the error bounds
                if(nextIndicesSize == lodIndices.GetSize() || nextIndicesSize == 0)
                    break;

                // while I could keep this LOD, it's too close to the last one (and it can't go below that due to constant error bound above)
			    if (nextIndicesSize >= size_t(double(lodIndices.GetSize()) * 0.95))
				    break;

                // Downsize the indices to the next indices size
                lodIndices.Downsize(nextIndicesSize);
 
                // Optimize the new vertex cache that was generated
                meshopt_optimizeVertexCache(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), vertices.GetSize());

                // since it starts from next lod accumulate the error
                lodError = BlitML::Max(lodError, nextError);
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

        // Default material
        newSurface.materialId = 0;

        // Add the resources to the global surface array so that it is added to the GPU buffer
        pResources->surfaces.PushBack(newSurface);
        pResources->primitiveVertexCounts.PushBack(static_cast<uint32_t>(vertices.GetSize()));
    }

    void LoadTestGeometry(RenderingResources* pResources)
    {
        LoadMeshFromObj(pResources, "Assets/Meshes/dragon.obj");
        LoadMeshFromObj(pResources, "Assets/Meshes/kitten.obj");
        LoadMeshFromObj(pResources, "Assets/Meshes/bunny.obj");
        LoadMeshFromObj(pResources, "Assets/Meshes/FinalBaseMesh.obj");
    }

    void CreateTestGameObjects(RenderingResources* pResources, uint32_t dc)
    {
        constexpr float ce_stressTestRandomTransformMultiplier = 3'000.f;
        #if defined(BLITZEN_RENDERING_STRESS_TEST)
        constexpr uint32_t drawCount = 4'500'000;
        #else
        uint32_t drawCount = dc;
        #endif

        BLIT_INFO("Loading Renderer Stress test with %i objects", drawCount)
        BLIT_WARN("If your machine cannot withstand this many objects, decrease the draw count or undef the stress test macro")

        pResources->objectCount += drawCount;// Normally the draw count differs from the game object count, but the engine is really simple at the moment
        pResources->transforms.Resize(pResources->transforms.GetSize() + 
        pResources->objectCount);// Every object has a different transform

        // Hardcode a large amount of male model mesh
        for(size_t i = 0; i < pResources->objectCount / 10; ++i)
        {
            BlitzenEngine::MeshTransform& transform = pResources->transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation(
            (float(rand()) / RAND_MAX) * ce_stressTestRandomTransformMultiplier,//x 
            (float(rand()) / RAND_MAX) * ce_stressTestRandomTransformMultiplier,//y
            (float(rand()) / RAND_MAX) * ce_stressTestRandomTransformMultiplier);//z
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
            BlitML::vec3 translation(
            (float(rand()) / RAND_MAX) * ce_stressTestRandomTransformMultiplier,//x 
            (float(rand()) / RAND_MAX) * ce_stressTestRandomTransformMultiplier,//y
            (float(rand()) / RAND_MAX) * ce_stressTestRandomTransformMultiplier);//z
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
            BlitML::vec3 translation(
            (float(rand()) / RAND_MAX) * ce_stressTestRandomTransformMultiplier,//x 
            (float(rand()) / RAND_MAX) * ce_stressTestRandomTransformMultiplier,//y
            (float(rand()) / RAND_MAX) * ce_stressTestRandomTransformMultiplier);//z
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
            BlitML::vec3 translation(
            (float(rand()) / RAND_MAX) * ce_stressTestRandomTransformMultiplier,//x 
            (float(rand()) / RAND_MAX) * ce_stressTestRandomTransformMultiplier,//y
            (float(rand()) / RAND_MAX) * ce_stressTestRandomTransformMultiplier);//z
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

            currentObject.meshIndex = 2;// Hardcode the bunny mesh for each object in this loop
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

    void CreateObliqueNearPlaneClippingTestObject(RenderingResources* pResources)
    {
        MeshTransform transform;
        transform.pos = BlitML::vec3(0);
        transform.scale = 10.f;
        transform.orientation = BlitML::QuatFromAngleAxis(BlitML::vec3(0), 0, 0);
        pResources->transforms.PushBack(transform);

        GameObject& currentObject = pResources->objects[pResources->objectCount++];
        currentObject.meshIndex = 3;// Hardcode the bunny mesh for each object in this loop
        currentObject.transformIndex = pResources->renderObjectCount;

        Mesh& mesh = pResources->meshes[currentObject.meshIndex];
        for(size_t i = 0; i < mesh.surfaceCount; ++i)
        {
            RenderObject& draw = pResources->renders[pResources->renderObjectCount++];
            draw.surfaceId = mesh.firstSurface + uint32_t(i);
            draw.transformId = currentObject.transformIndex;
        }

        const uint32_t nonReflectiveDrawCount = 1000;
        const uint32_t start = pResources->objectCount;
        pResources->objectCount += nonReflectiveDrawCount;
        pResources->transforms.Resize(pResources->transforms.GetSize() + pResources->objectCount);

        for (size_t i = start; i < pResources->objectCount; ++i)
        {
            BlitzenEngine::MeshTransform& tr = pResources->transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation(
            (float(rand()) / RAND_MAX) * 100,//x 
            (float(rand()) / RAND_MAX) * 100,//y
            (float(rand()) / RAND_MAX) * 100);//z
            tr.pos = translation;
            tr.scale = 1.f;

            // Loading random orientation. Normally you would get this from the game object
            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
            (float(rand()) / RAND_MAX) * 2 - 1, // y
            (float(rand()) / RAND_MAX) * 2 - 1); // z
		    float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            tr.orientation = orientation;

            GameObject& currentObject = pResources->objects[i];

            currentObject.meshIndex = 1;// Hardcode the kitten mesh for each object in this loop
            currentObject.transformIndex = static_cast<uint32_t>(i);// Transform index is the same as the object index

            Mesh& m = pResources->meshes[currentObject.meshIndex];
            for(size_t i = 0; i < m.surfaceCount; ++i)
            {
                RenderObject& draw = pResources->renders[pResources->renderObjectCount++];
                draw.surfaceId = m.firstSurface + uint32_t(i);
                draw.transformId = currentObject.transformIndex;
            }
        }
    }

    // Calls some test functions to load a scene that tests the renderer's geometry rendering
    void LoadGeometryStressTest(RenderingResources* pResources, uint32_t drawCount)
    {
        LoadTestGeometry(pResources);
        CreateTestGameObjects(pResources, drawCount);
    }

    // Takes a path to a gltf file and loads the resources needed to render the scene
    // This function uses the cgltf library to load a .glb or .gltf scene
    // The repository can be found on https://github.com/jkuhlmann/cgltf
    uint8_t LoadGltfScene(RenderingResources* pResources, const char* path, void* rendererData)
    {
        if (pResources->renderObjectCount >= ce_maxRenderObjects)
        {
            BLIT_WARN("BLITZEN_MAX_DRAW_OBJECT already reached, no more geometry can be loaded. GLTF LOADING FAILED!")
                return 0;
        }

        cgltf_options options = {};

        cgltf_data* pData = nullptr;

        cgltf_result res = cgltf_parse_file(&options, path, &pData);
        if (res != cgltf_result_success)
        {
            BLIT_WARN("Failed to load gltf file: %s", path)
                return 0;
        }

        // Struct only used for cgltf_data pointer, will call its specialized free function when done
        struct CgltfScope
        {
            cgltf_data* pData;

            inline ~CgltfScope() { cgltf_free(pData); }
        };

        CgltfScope cgltfScope{ pData };

        res = cgltf_load_buffers(&options, pData, path);
        if (res != cgltf_result_success)
        {
            return 0;
        }

        res = cgltf_validate(pData);
        if (res != cgltf_result_success)
        {
            return 0;
        }

        BLIT_INFO("Loading GLTF scene from file: %s", path)

            // Defining a lambda here for finding the accessor since it will probably not be used outside of this
            auto findAccessor = [](const cgltf_primitive* prim, cgltf_attribute_type type, cgltf_int index = 0) {
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

        // Before loading textures save the previous texture size, to use for indexing
        size_t previousTextureSize = pResources->textureCount;

        BLIT_INFO("Loading textures")

            // I had to fold and use the STL
        BlitCL::DynamicArray<std::string> texturePaths(pData->textures_count);
        for (size_t i = 0; i < pData->textures_count; ++i)
        {
            cgltf_texture* texture = &(pData->textures[i]);
            if (!texture->image)
                break;

            cgltf_image* image = texture->image;
            if (!image->uri)
                break;

            std::string ipath = path;
            std::string::size_type pos = ipath.find_last_of('/');
            if (pos == std::string::npos)
                ipath = "";
            else
                ipath = ipath.substr(0, pos + 1);

            std::string uri = image->uri;
            uri.resize(cgltf_decode_uri(&uri[0]));
            std::string::size_type dot = uri.find_last_of('.');

            if (dot != std::string::npos)
                uri.replace(dot, uri.size() - dot, ".dds");

            texturePaths[i] = ipath + uri;
        }

        auto pRenderer = reinterpret_cast<RendererPtrType>(rendererData);
        for (auto path : texturePaths)
        {
            LoadTextureFromFile(pResources, path.c_str(), path.c_str(), pRenderer);
        }

        BLIT_INFO("Loading materials")

        // Saves the previous material count
        size_t previousMaterialCount = pResources->materialCount;
        // Creates one BlitzenEngine::Material for each material in the gltf
        for (size_t i = 0; i < pData->materials_count; ++i)
        {
            cgltf_material& cgltf_mat = pData->materials[i];

            Material& mat = pResources->materials[pResources->materialCount++];
            mat.materialId = static_cast<uint32_t>(pResources->materialCount - 1);

            mat.albedoTag = cgltf_mat.pbr_metallic_roughness.base_color_texture.texture ?
                uint32_t(previousTextureSize + cgltf_texture_index(pData, cgltf_mat.pbr_metallic_roughness.base_color_texture.texture))
                : cgltf_mat.pbr_specular_glossiness.diffuse_texture.texture ?
                uint32_t(previousTextureSize + cgltf_texture_index(pData, cgltf_mat.pbr_specular_glossiness.diffuse_texture.texture))
                : 0;

            mat.normalTag =
                cgltf_mat.normal_texture.texture ?
                uint32_t(previousTextureSize + cgltf_texture_index(pData, cgltf_mat.normal_texture.texture))
                : 0;

            mat.specularTag =
                cgltf_mat.pbr_specular_glossiness.specular_glossiness_texture.texture ?
                uint32_t(previousTextureSize + cgltf_texture_index(pData, cgltf_mat.pbr_specular_glossiness.specular_glossiness_texture.texture))
                : 0;

            mat.emissiveTag =
                cgltf_mat.emissive_texture.texture ?
                uint32_t(previousTextureSize + cgltf_texture_index(pData, cgltf_mat.emissive_texture.texture))
                : 0;

        }

        BLIT_INFO("Loading meshes and primitives")

            // The surface indices is a list of the first surface of each mesh. Used to create the render object struct
            BlitCL::DynamicArray<uint32_t> surfaceIndices(pData->meshes_count);

        for (size_t i = 0; i < pData->meshes_count; ++i)
        {
            // Get the current mesh
            const cgltf_mesh& mesh = pData->meshes[i];

            // Find the first surface of the current mesh. 
            // It is important for the mesh struct and to save the data for later to create the render objects
            uint32_t firstSurface = static_cast<uint32_t>(pResources->surfaces.GetSize());

            // Give the new mesh the surface that it owns and increment the mesh count
            pResources->meshes[pResources->meshCount].firstSurface = firstSurface;
            pResources->meshes[pResources->meshCount].surfaceCount = static_cast<uint32_t>(mesh.primitives_count);
            pResources->meshCount++;

            // Pass the first surface here so that it can be accessed by the nodes
            surfaceIndices[i] = firstSurface;

            for (size_t j = 0; j < mesh.primitives_count; ++j)
            {
                const cgltf_primitive& prim = mesh.primitives[j];

                // Skip primitives that are not triangles
                if (prim.type != cgltf_primitive_type_triangles || !prim.indices)
                    continue;

                size_t vertexCount = prim.attributes[0].data->count;

                BlitCL::DynamicArray<Vertex> vertices(vertexCount);

                // Will temporarily hold each aspect of the vertices (pos, tangent, normals, uvMaps) from the primitive
                BlitCL::DynamicArray<float> scratch(vertexCount * 4);

                if (const cgltf_accessor* pos = cgltf_find_accessor(&prim, cgltf_attribute_type_position, 0))
                {
                    // No choice but to assert here, as some data might already have been loaded
                    BLIT_ASSERT(cgltf_num_components(pos->type) == 3);

                    cgltf_accessor_unpack_floats(pos, scratch.Data(), vertexCount * 3);
                    for (size_t j = 0; j < vertexCount; ++j)
                    {
                        vertices[j].position = BlitML::vec3(scratch[j * 3 + 0], scratch[j * 3 + 1], scratch[j * 3 + 2]);
                    }
                }

                if (const cgltf_accessor* nrm = cgltf_find_accessor(&prim, cgltf_attribute_type_normal, 0))
                {
                    BLIT_ASSERT(cgltf_num_components(nrm->type) == 3);

                    cgltf_accessor_unpack_floats(nrm, scratch.Data(), vertexCount * 3);
                    for (size_t j = 0; j < vertexCount; ++j)
                    {
                        vertices[j].normalX = static_cast<uint8_t>(scratch[j * 3 + 0] * 127.f + 127.5f);
                        vertices[j].normalY = static_cast<uint8_t>(scratch[j * 3 + 1] * 127.f + 127.5f);
                        vertices[j].normalZ = static_cast<uint8_t>(scratch[j * 3 + 2] * 127.f + 127.5f);
                    }
                }

                if (const cgltf_accessor* tang = cgltf_find_accessor(&prim, cgltf_attribute_type_tangent, 0))
                {
                    BLIT_ASSERT(cgltf_num_components(tang->type) == 4)

                        cgltf_accessor_unpack_floats(tang, scratch.Data(), vertexCount * 4);
                    for (size_t j = 0; j < vertexCount; ++j)
                    {
                        vertices[j].tangentX = uint8_t(scratch[j * 4 + 0] * 127.f + 127.5f);
                        vertices[j].tangentY = uint8_t(scratch[j * 4 + 1] * 127.f + 127.5f);
                        vertices[j].tangentZ = uint8_t(scratch[j * 4 + 2] * 127.f + 127.5f);
                        vertices[j].tangentW = uint8_t(scratch[j * 4 + 3] * 127.f + 127.5f);
                    }
                }

                if (const cgltf_accessor* tex = cgltf_find_accessor(&prim, cgltf_attribute_type_texcoord, 0))
                {
                    BLIT_ASSERT(cgltf_num_components(tex->type) == 2);
                    cgltf_accessor_unpack_floats(tex, scratch.Data(), vertexCount * 2);
                    for (size_t j = 0; j < vertexCount; ++j)
                    {
                        vertices[j].uvX = meshopt_quantizeHalf(scratch[j * 2 + 0]);
                        vertices[j].uvY = meshopt_quantizeHalf(scratch[j * 2 + 1]);
                    }
                }

                BlitCL::DynamicArray<uint32_t> indices(prim.indices->count);
                cgltf_accessor_unpack_indices(prim.indices, indices.Data(), 4, indices.GetSize());

                LoadPrimitiveSurface(pResources, vertices, indices);

                // Get the material index and pass it to the surface if there is material index
                if (prim.material)
                {
                    pResources->surfaces.Back().materialId =
                        pResources->materials[previousMaterialCount + cgltf_material_index(pData, prim.material)].materialId;

                    if (prim.material->alpha_mode != cgltf_alpha_mode_opaque)
                        pResources->surfaces.Back().postPass = 1;
                }
            }
        }

        BLIT_INFO("Loading scene nodes")

            for (size_t i = 0; i < pData->nodes_count; ++i)
            {
                const cgltf_node* node = &(pData->nodes[i]);
                if (node->mesh)
                {
                    // Gets the model matrix
                    float matrix[16];
                    cgltf_node_transform_world(node, matrix);

                    // Uses these float arrays to hold the decomposed matrix
                    float translation[3];
                    float rotation[4];
                    float scale[3];

                    // Decomposes the model transform and tranlates the data to the engine's transform structure
                    BlitML::decomposeTransform(translation, rotation, scale, matrix);
                    MeshTransform transform;
                    transform.pos = BlitML::vec3(translation[0], translation[1], translation[2]);
                    transform.scale = BlitML::Max(scale[0], BlitML::Max(scale[1], scale[2]));
                    transform.orientation = BlitML::quat(rotation[0], rotation[1], rotation[2], rotation[3]);

                    // TODO: better warnings for non-uniform or negative scale

                    // Hold the offset of the first surface of the mesh and the transform id to give to the render objects
                    uint32_t surfaceOffset = surfaceIndices[cgltf_mesh_index(pData, node->mesh)];
                    uint32_t transformId = static_cast<uint32_t>(pResources->transforms.GetSize());

                    for (unsigned int j = 0; j < node->mesh->primitives_count; ++j)
                    {
                        // If the gltf goes over BLITZEN_MAX_DRAW_OBJECTS after already loading resources, I have no choice but to assert
                        BLIT_ASSERT_MESSAGE(pResources->renderObjectCount <= ce_maxRenderObjects, "While Loading a GLTF, \
                    additional geometry was loaded which surpassed the BLITZEN_MAX_DRAW_OBJECT limiter value")

                            RenderObject& current = pResources->renders[pResources->renderObjectCount];
                        current.surfaceId = surfaceOffset + j;
                        current.transformId = transformId;

                        // Increment the render object count
                        pResources->renderObjectCount++;
                    }

                    pResources->transforms.PushBack(transform);
                }
            }

        return 1;
    }
}