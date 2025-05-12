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
#include "objparser.h"
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
	RenderingResources* RenderingResources::s_pResources = nullptr;

    RenderingResources::RenderingResources(void* rendererData)
    {
		s_pResources = this;

        auto pRenderer = reinterpret_cast<RendererPtrType>(rendererData);

        // Defaults
        LoadTextureFromFile("Assets/Textures/base_baseColor.dds","dds_texture_default", pRenderer);
        DefineMaterial(BlitML::vec4{ 0.1f }, 65.f, "dds_texture_default", "unknown", "loaded_material");
        LoadMeshFromObj("Assets/Meshes/bunny.obj", ce_defaultMeshName);
    }

    void RenderingResources::GenerateHlslVertices()
    {
        if (m_hlslVtxs.GetSize())
        {
            return;
        }

        for (size_t i = 0; i < m_vertices.GetSize(); ++i)
        {
            const auto& classic = m_vertices[i];
            HlslVtx hlsl{};

            hlsl.position = classic.position;

            hlsl.mappingU = classic.uvX; 
            hlsl.mappingV = classic.uvY;

            hlsl.normals = classic.normalX << 24 | classic.normalY << 16 | classic.normalZ << 8 | classic.normalW;
            hlsl.tangents = classic.tangentX << 24 | classic.tangentY << 16 | classic.tangentZ << 8 | classic.tangentW;

            m_hlslVtxs.PushBack(hlsl);
        }
    }

    void RenderingResources::DefineMaterial(const BlitML::vec4& diffuseColor, float shininess, 
        const char* diffuseMapName, const char* specularMapName, const char* materialName)
    {
        auto& mat = m_materials[m_materialCount];

        mat.albedoTag = 0; //pResources->textureTable[diffuseMapName].textureTag;
        mat.normalTag = 0; //pResources->textureTable[specularMapName].textureTag;
        mat.specularTag = 0;
        mat.emissiveTag = 0;

        mat.materialId = m_materialCount;

        m_materialTable.Insert(materialName, mat);
        m_materialCount++;
    }

    // Loads cluster using the meshoptimizer library
    size_t RenderingResources::GenerateClusters(BlitCL::DynamicArray<Vertex>& inVertices, 
        BlitCL::DynamicArray<uint32_t>& inIndices, uint32_t vertexOffset)
    {
        BLIT_INFO("Generating clusters");

        const size_t maxVertices = 64;
        const size_t maxTriangles = 124;
        const float coneWeight = 0.25f;

        BlitCL::DynamicArray<meshopt_Meshlet> akMeshlets{ meshopt_buildMeshletsBound(inIndices.GetSize(), maxVertices, maxTriangles) };
        BlitCL::DynamicArray<unsigned int> meshletVertices{ akMeshlets.GetSize() * maxVertices };
        BlitCL::DynamicArray<unsigned char> meshletTriangles{ akMeshlets.GetSize() * maxTriangles * 3 };

        akMeshlets.Resize(meshopt_buildMeshlets(akMeshlets.Data(), meshletVertices.Data(), meshletTriangles.Data(), inIndices.Data(), inIndices.GetSize(), 
            &inVertices[0].position.x, inVertices.GetSize(), sizeof(Vertex), maxVertices, maxTriangles, coneWeight));


        for(size_t i = 0; i < akMeshlets.GetSize(); ++i)
        {
            auto& meshlet = akMeshlets[i];

            meshopt_optimizeMeshlet(&meshletVertices[meshlet.vertex_offset], 
                &meshletTriangles[meshlet.triangle_offset], 
                meshlet.triangle_count, meshlet.vertex_count);

            /*auto dataOffset = m_clusterIndices.GetSize();
            for(unsigned int i = 0; i < meshlet.vertex_count; ++i)
            {
                m_clusterIndices.PushBack(meshletVertices[meshlet.vertex_offset + i]);
            }
            auto indexGroups = reinterpret_cast<unsigned int*>(
                &meshletTriangles[0] + meshlet.triangle_offset);
            unsigned int indexGroupCount = meshlet.triangle_count * 3;
            for(unsigned int i = 0; i < indexGroupCount; ++i)
            {
                m_clusterIndices.PushBack(indexGroups[size_t(i)]);
            }*/

            const unsigned int* vertexLookup = &meshletVertices[meshlet.vertex_offset];
            const unsigned char* triangles = &meshletTriangles[meshlet.triangle_offset];
            auto dataOffset = m_clusterIndices.GetSize();
            for (unsigned int t = 0; t < meshlet.triangle_count; ++t)
            {
                // Each triangle has 3 indices into the local meshlet vertex array
                for (int j = 0; j < 3; ++j)
                {
                    unsigned int localIndex = triangles[t * 3 + j];
                    unsigned int globalIndex = vertexLookup[localIndex] + vertexOffset;
                    m_clusterIndices.PushBack(globalIndex);
                }
            }

            auto bounds = meshopt_computeMeshletBounds(&meshletVertices[meshlet.vertex_offset], 
            &meshletTriangles[meshlet.triangle_offset], meshlet.triangle_count, &inVertices[0].position.x, inVertices.GetSize(), sizeof(Vertex));

            Cluster cluster {};
            cluster.dataOffset = static_cast<uint32_t>(dataOffset);
            cluster.triangleCount = meshlet.triangle_count;
            cluster.vertexCount = meshlet.vertex_count;

            cluster.center = BlitML::vec3(bounds.center[0], bounds.center[1], bounds.center[2]);
            cluster.radius = bounds.radius;
            cluster.coneAxisX = bounds.cone_axis_s8[0];
		    cluster.coneAxisY = bounds.cone_axis_s8[1];
		    cluster.coneAxisZ = bounds.cone_axis_s8[2];
		    cluster.coneCutoff = bounds.cone_cutoff_s8; 

            m_clusters.PushBack(cluster);
        }

        return akMeshlets.GetSize();
    }

    // Called  to create a Primitive Surface resource for the renderer
    void RenderingResources::LoadPrimitiveSurface(BlitCL::DynamicArray<Vertex>& surfaceVertices, 
        BlitCL::DynamicArray<uint32_t>& surfaceIndices)
    {
        // Optimize vertices and indices using meshoptimizer
        meshopt_optimizeVertexCache(surfaceIndices.Data(), surfaceIndices.Data(), surfaceIndices.GetSize(), surfaceVertices.GetSize());
        meshopt_optimizeVertexFetch(surfaceVertices.Data(), surfaceIndices.Data(), surfaceIndices.GetSize(), surfaceVertices.Data(), 
            surfaceVertices.GetSize(), sizeof(Vertex));

        PrimitiveSurface newSurface{};
        newSurface.vertexOffset = static_cast<uint32_t>(m_vertices.GetSize());
        m_vertices.AppendArray(surfaceVertices);

        BLIT_INFO("Generating Level Of Detail Indices");
        AutomaticLevelOfDetailGenration(newSurface, surfaceVertices, surfaceIndices);

        BLIT_INFO("Generating bounding sphere");
        GenerateBoundingSphere(newSurface, surfaceVertices, surfaceIndices);

        // TODO: Add logic for material without relying on gltf
        newSurface.materialId = 0;

		// Adds the surface to the global surfaces array
        m_surfaces.PushBack(newSurface);
        m_primitiveVertexCounts.PushBack(static_cast<uint32_t>(m_vertices.GetSize()));
        // Default, if a caller wants transparency, they should handle it
		bTransparencyList.PushBack({ false });
    }

    void RenderingResources::AutomaticLevelOfDetailGenration(PrimitiveSurface& surface,
        BlitCL::DynamicArray<Vertex>& surfaceVertices, BlitCL::DynamicArray<uint32_t>& surfaceIndices)
    {
        // Automatic LOD generation helpers
        BlitCL::DynamicArray<BlitML::vec3> normals{ surfaceVertices.GetSize() };
        for (size_t i = 0; i < surfaceVertices.GetSize(); ++i)
        {
            Vertex& v = surfaceVertices[i];
            normals[i] = BlitML::vec3(v.normalX / 127.f - 1.f,
                v.normalY / 127.f - 1.f,
                v.normalZ / 127.f - 1.f);
        }
        float lodScale = meshopt_simplifyScale(&surfaceVertices[0].position.x,
            surfaceVertices.GetSize(), sizeof(Vertex));
        float lodError = 0.f;
        float normalWeights[3] = { 1.f, 1.f, 1.f };

        // Pass the original loaded indices of the surface to the new lod indices
        BlitCL::DynamicArray<uint32_t> lodIndices{ surfaceIndices };
        BlitCL::DynamicArray<uint32_t> allLodIndices;

        surface.lodOffset = static_cast<uint32_t>(m_lods.GetSize());
        while (surface.lodCount < Ce_MaxLodCountPerSurface)
        {
            surface.lodCount++;
            BLIT_INFO("Generating LOD %d", surface.lodCount);

            // Instancing
            LodInstanceCounter lodInstanceCounter{};
            lodInstanceCounter.instanceOffset = uint32_t(m_lods.GetSize() * Ce_MaxInstanceCountPerLOD);
            m_lodInstanceList.PushBack(lodInstanceCounter);

            LodData lod{};
            lod.firstIndex = static_cast<uint32_t>(m_indices.GetSize() + allLodIndices.GetSize());
            lod.indexCount = static_cast<uint32_t>(lodIndices.GetSize());

            // TODO: Might want to make LODs include one or the other, indices and clusters are not used together
            lod.clusterOffset = static_cast<uint32_t>(m_clusters.GetSize());
            lod.clusterCount = Ce_BuildClusters ? static_cast<uint32_t>(GenerateClusters(surfaceVertices, lodIndices, surface.vertexOffset)) : 0;

            lod.error = lodError * lodScale;
            m_lods.PushBack(lod);

            // Adds current lod indices
            allLodIndices.AppendArray(lodIndices);
            
            // Starts generating the next level of detail
            if (surface.lodCount < Ce_MaxLodCountPerSurface)
            {
                auto nextIndicesTarget = static_cast<size_t>((double(lodIndices.GetSize()) * 0.65) / 3) * 3;
                const float maxError = 1e-1f;
                float nextError = 0.f;

                // Gets the size of the next level of detail
                auto nextIndicesSize = meshopt_simplifyWithAttributes(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), &surfaceVertices[0].position.x, 
                    surfaceVertices.GetSize(), sizeof(Vertex), &normals[0].x, sizeof(BlitML::vec3), normalWeights, 3, nullptr, nextIndicesTarget, maxError, 
                    0, &nextError);

                if (nextIndicesSize > lodIndices.GetSize())
                {
                    BLIT_ERROR("LOD generation failed");
                    break;
                }
                // Reached the error bounds
                if (nextIndicesSize == lodIndices.GetSize() || nextIndicesSize == 0)
                {
                    break;
                }
                // while I could keep this LOD, it's too close to the last one 
                // (and it can't go below that due to constant error bound above)
                if (nextIndicesSize >= size_t(double(lodIndices.GetSize()) * 0.95))
                {
                    break;
                }

                // Resize and optimize
                lodIndices.Resize(nextIndicesSize);
                meshopt_optimizeVertexCache(lodIndices.Data(), lodIndices.Data(), lodIndices.GetSize(), surfaceVertices.GetSize());

                // since it starts from next lod accumulate the error
                lodError = BlitML::Max(lodError, nextError);
            }
        }

        if (surface.lodCount > Ce_MaxLodCountPerSurface)
        {
            BLIT_ERROR("A surface has loaded too many LODs");
        }

        // Adds vertex offset before appending all lods to the global indices array
		auto vertexOffset = surface.vertexOffset;
		for (auto& index : allLodIndices)
		{
            index += vertexOffset;
		}
		m_indices.AppendArray(allLodIndices);
    }

    void RenderingResources::GenerateBoundingSphere(PrimitiveSurface& surface,
        BlitCL::DynamicArray<Vertex>& surfaceVertices,
        BlitCL::DynamicArray<uint32_t>& surfaceIndices)
    {
        BlitML::vec3 center{ 0.f };
        for (size_t i = 0; i < surfaceVertices.GetSize(); ++i)
        {
            center = center + surfaceVertices[i].position;
        }
        center = center / static_cast<float>(surfaceVertices.GetSize());

        // Bounding sphere radius
        float radius = 0;
        for (size_t i = 0; i < surfaceVertices.GetSize(); ++i)
        {
            const auto& pos = surfaceVertices[i].position;
            radius = BlitML::Max(radius, BlitML::Distance(center, BlitML::vec3(pos.x, pos.y, pos.z)));
        }
        surface.center = center;
        surface.radius = radius;
    }


    void RenderingResources::GenerateTangents(BlitCL::DynamicArray<BlitzenEngine::Vertex>& vertices, 
        BlitCL::DynamicArray<uint32_t>& indices
    ) 
    {
        for (size_t i = 0; i < indices.GetSize(); i += 3) 
        {
            auto i0 = indices[i + 0];
            auto i1 = indices[i + 1];
            auto i2 = indices[i + 2];

            auto edge1 = vertices[i1].position - vertices[i0].position;
            auto edge2 = vertices[i2].position - vertices[i0].position;

            auto deltaU1 = float(vertices[i1].uvX - vertices[i0].uvX);
            auto deltaV1 = float(vertices[i1].uvY - vertices[i0].uvY);

            auto deltaU2 = float(vertices[i2].uvX - vertices[i0].uvX);
            auto deltaV2 = float(vertices[i2].uvY - vertices[i0].uvY);

            float dividend = (deltaU1 * deltaV2 - deltaU2 * deltaV1);

            float fc = 1.0f / dividend;

            BlitML::vec3 tangent
            {
                (fc * (deltaV2 * edge1.x - deltaV1 * edge2.x)),
                (fc * (deltaV2 * edge1.y - deltaV1 * edge2.y)),
                (fc * (deltaV2 * edge1.z - deltaV1 * edge2.z)) 
            };

            BlitML::Normalize(tangent);

            float sx = deltaU1, sy = deltaU2;

            float tx = deltaV1, ty = deltaV2;

            float handedness = ((tx * sy - ty * sx) < 0.0f) ? -1.0f : 1.0f;

            BlitML::vec4 t4{ tangent, handedness };

            vertices[i0].tangentX = uint8_t(t4.x * 127.f + 127.5f);
			vertices[i0].tangentY = uint8_t(t4.y * 127.f + 127.5f);
			vertices[i0].tangentZ = uint8_t(t4.z * 127.f + 127.5f);
			vertices[i0].tangentW = uint8_t(t4.w * 127.f + 127.5f);

            vertices[i1].tangentX = uint8_t(t4.x * 127.f + 127.5f);
			vertices[i1].tangentY = uint8_t(t4.y * 127.f + 127.5f);
			vertices[i1].tangentZ = uint8_t(t4.z * 127.f + 127.5f);
			vertices[i1].tangentW = uint8_t(t4.w * 127.f + 127.5f);

            vertices[i2].tangentX = uint8_t(t4.x * 127.f + 127.5f);
            vertices[i2].tangentY = uint8_t(t4.y * 127.f + 127.5f);
            vertices[i2].tangentZ = uint8_t(t4.z * 127.f + 127.5f);
            vertices[i2].tangentW = uint8_t(t4.w * 127.f + 127.5f);
        }
    }

    bool RenderingResources::CreateRenderObject(uint32_t transformId, uint32_t surfaceId)
    {
        if (renderObjectCount >= ce_maxRenderObjects)
        {
            BLIT_ERROR("Max render object count reached");
            return false;
        }

        if (!bTransparencyList[surfaceId].b)
        {
            auto& current = renders[renderObjectCount];
            current.surfaceId = surfaceId;
            current.transformId = transformId;

            renderObjectCount++;
        }
        else
        {
            RenderObject current{};
            current.surfaceId = surfaceId;
            current.transformId = transformId;
            m_transparentRenders.PushBack(current);
        }

        return true;
    }

    uint8_t RenderingResources::LoadMeshFromObj(const char* filename, const char* meshName)
    {
        // The function should return if the engine will go over the max allowed mesh assets
        if(meshCount > ce_maxMeshCount)
        {
            BLIT_ERROR("Max mesh count: ( %i ) reached!", ce_maxMeshCount)
            return 0;
        }

        BLIT_INFO("Loading obj model form file: %s", filename);

        // Get the current mesh and give it the size surface array as its first surface index
        auto& currentMesh = meshes[meshCount];
        currentMesh.firstSurface = static_cast<uint32_t>(m_surfaces.GetSize());
		currentMesh.meshId = uint32_t(meshCount);
        meshMap.Insert(meshName, currentMesh);

        ObjFile file;
        if (!objParseFile(file, filename))
        {
            return 0;
        }

        size_t indexCount = file.f_size / 3;

        BlitCL::DynamicArray<Vertex> triangleVertices(indexCount);

        BLIT_INFO("Loading vertices and indices");

        for(size_t i = 0; i < indexCount; ++i)
        {
            auto& vtx = triangleVertices[i];

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

		    vtx.uvX = vertexTextureIndex < 0 ? 0.f : file.vt[vertexTextureIndex * 3 + 0];
		    vtx.uvY = vertexTextureIndex < 0 ? 0.f : file.vt[vertexTextureIndex * 3 + 1];
        }

        // Creates indices for the obj's vertices using meshopt
        BlitCL::DynamicArray<uint32_t> remap(indexCount);
		size_t vertexCount = meshopt_generateVertexRemap(remap.Data(), 0, indexCount, triangleVertices.Data(), indexCount, sizeof(Vertex));
        BlitCL::DynamicArray<uint32_t> indices(indexCount);
        BlitCL::DynamicArray<Vertex> vertices(vertexCount);
        meshopt_remapVertexBuffer(vertices.Data(), triangleVertices.Data(), indexCount, sizeof(Vertex), remap.Data());
		meshopt_remapIndexBuffer(indices.Data(), 0, indexCount, remap.Data());

		GenerateTangents(vertices, indices);

        BLIT_INFO("Creating surface");
        LoadPrimitiveSurface(vertices, indices);

        currentMesh.surfaceCount++;// Increment the surface count
        ++meshCount;// Increment the mesh count

        return 1;
    }

    uint32_t RenderingResources::AddRenderObjectsFromMesh(uint32_t meshId,
        const BlitzenEngine::MeshTransform& transform, bool bDynamic)
    {
        auto& mesh = meshes[meshId];
        if (renderObjectCount + mesh.surfaceCount >= ce_maxRenderObjects)
        {
            BLIT_ERROR("Adding renderer objects from mesh will exceed the render object count");
            // This is really strange behavior for this function, since the caller expects a transform id
            // BE CAREFUL
            return 0;
        }

        auto transformId = static_cast<uint32_t>(transforms.GetSize());
        if (bDynamic)
        {
            transformId = dynamicTransformCount;
            transforms[dynamicTransformCount++] = transform;
        }
        else
        {
            transforms.PushBack(transform);
        }
        
        for (auto i = mesh.firstSurface; i < mesh.firstSurface + mesh.surfaceCount; ++i)
        {
			CreateRenderObject(transformId, i);
        }

        return transformId;
    }

    void RenderingResources::AddPrimitivesFromGltf(const cgltf_data* pGltfData, const cgltf_mesh& gltfMesh, uint32_t previousMaterialCount)
    {
        for (size_t j = 0; j < gltfMesh.primitives_count; ++j)
        {
            const cgltf_primitive& prim = gltfMesh.primitives[j];

            // Skips primitives that do not consist of triangles
            if (prim.type != cgltf_primitive_type_triangles || !prim.indices)
            {
                BLIT_ERROR("Blitzen supports only primitives with cgltf_primitive_type_triangles flags set and with indices");
                continue;
            }

            size_t vertexCount = prim.attributes[0].data->count;
            BlitCL::DynamicArray<Vertex> vertices{ vertexCount };
            BlitCL::DynamicArray<float> scratch{ vertexCount * 4 };

            // Vertex positions
            if (const cgltf_accessor* pos = cgltf_find_accessor(&prim, cgltf_attribute_type_position, 0))
            {
                BLIT_ASSERT(cgltf_num_components(pos->type) == 3);

                cgltf_accessor_unpack_floats(pos, scratch.Data(), vertexCount * 3);
                for (size_t j = 0; j < vertexCount; ++j)
                {
                    vertices[j].position = BlitML::vec3(scratch[j * 3 + 0], scratch[j * 3 + 1], scratch[j * 3 + 2]);
                }
            }

            // Vertex normals
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

            // Vertex tangents
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
                    vertices[j].uvX = scratch[j * 2 + 0];
                    vertices[j].uvY = scratch[j * 2 + 1];
                }
            }

            BlitCL::DynamicArray<uint32_t> indices(prim.indices->count);
            cgltf_accessor_unpack_indices(prim.indices, indices.Data(), 4, indices.GetSize());

            LoadPrimitiveSurface(vertices, indices);

            // Get the material index and pass it to the surface if there is material index
            if (prim.material)
            {
                m_surfaces.Back().materialId = m_materials[previousMaterialCount + cgltf_material_index(pGltfData, prim.material)].materialId;

                if (prim.material->alpha_mode != cgltf_alpha_mode_opaque)
                {
					bTransparencyList[m_surfaces.GetSize() - 1].b = true;
                }
            }
        }
    }

    void RenderingResources::CreateRenderObjectsFromGltffNodes(cgltf_data* pGltfData,
        const BlitCL::DynamicArray<uint32_t>& surfaceIndices)
    {
        for (size_t i = 0; i < pGltfData->nodes_count; ++i)
        {
            const cgltf_node* node = &(pGltfData->nodes[i]);
            if (node->mesh)
            {
                // Gets the model matrix
                float matrix[16];
                cgltf_node_transform_world(node, matrix);

                // Have to decompose the transform, since blitzen does not use matrix for transform
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
                auto surfaceOffset = surfaceIndices[cgltf_mesh_index(pGltfData, node->mesh)];
                auto transformId = static_cast<uint32_t>(transforms.GetSize());
                for (size_t j = 0; j < node->mesh->primitives_count; ++j)
                {
                    CreateRenderObject(transformId, surfaceOffset + static_cast<uint32_t>(j));
                }
                transforms.PushBack(transform);
            }
        }
    }

    void RenderingResources::LoadGltfMaterials(const cgltf_data* pGltfData, uint32_t previousMaterialCount, 
        uint32_t previousTextureCount)
    {
        for (size_t i = 0; i < pGltfData->materials_count; ++i)
        {
            auto& cgltf_mat = pGltfData->materials[i];

            auto& mat = m_materials[m_materialCount++];
            mat.materialId = m_materialCount - 1;

            mat.albedoTag = cgltf_mat.pbr_metallic_roughness.base_color_texture.texture ?
                uint32_t(previousTextureCount + cgltf_texture_index(pGltfData,
                    cgltf_mat.pbr_metallic_roughness.base_color_texture.texture))
                : cgltf_mat.pbr_specular_glossiness.diffuse_texture.texture ?
                uint32_t(previousTextureCount + cgltf_texture_index(pGltfData,
                    cgltf_mat.pbr_specular_glossiness.diffuse_texture.texture))
                : 0;

            mat.normalTag =
                cgltf_mat.normal_texture.texture ?
                uint32_t(previousTextureCount + cgltf_texture_index(pGltfData,
                    cgltf_mat.normal_texture.texture))
                : 0;

            mat.specularTag =
                cgltf_mat.pbr_specular_glossiness.specular_glossiness_texture.texture ?
                uint32_t(previousTextureCount + cgltf_texture_index(pGltfData,
                    cgltf_mat.pbr_specular_glossiness.specular_glossiness_texture.texture))
                : 0;

            mat.emissiveTag =
                cgltf_mat.emissive_texture.texture ?
                uint32_t(previousTextureCount + cgltf_texture_index(pGltfData,
                    cgltf_mat.emissive_texture.texture))
                : 0;

        }
    }






    void LoadTestGeometry(RenderingResources* pResources)
    {
        pResources->LoadMeshFromObj("Assets/Meshes/dragon.obj", "dragon");
        pResources->LoadMeshFromObj("Assets/Meshes/kitten.obj", "kitten");
        pResources->LoadMeshFromObj("Assets/Meshes/FinalBaseMesh.obj", "human");
    }

    void RenderingResources::CreateSingleObjectForTesting()
    {
		// Create a single object for testing
		MeshTransform transform;
		transform.pos = BlitML::vec3(BlitzenEngine::ce_initialCameraX, BlitzenEngine::ce_initialCameraY, BlitzenEngine::ce_initialCameraZ);
		transform.scale = 10.f;
		transform.orientation = BlitML::QuatFromAngleAxis(BlitML::vec3(0), 0, 0);
        AddRenderObjectsFromMesh(meshMap["kitten"].meshId, transform, false);
		
    }

    void RandomizeTransform(MeshTransform& transform, float multiplier, float scale)
    {
        transform.pos = BlitML::vec3((float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier);

        transform.scale = scale;

        transform.orientation = BlitML::QuatFromAngleAxis(BlitML::vec3((float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1),
            BlitML::Radians((float(rand()) / RAND_MAX) * 90.f), 0);
    }

    static void CreateRenderObjectWithRandomTransform(uint32_t meshId, RenderingResources* pResources,
        float randomTransformMultiplier, float scale)
    {
		if (pResources->renderObjectCount >= ce_maxRenderObjects)
		{
            BLIT_WARN("Max render object count reached");
			return;
		}

		// Get the mesh used by the current game object
		auto& currentMesh = pResources->meshes[meshId];
        if (currentMesh.surfaceCount > 1)
		{
            BLIT_WARN("Only meshes with one primitive are allowed");
			return;
		}

        // Creates a new transform, radomizes and creates render object based on it
        BlitzenEngine::MeshTransform transform;
        RandomizeTransform(transform, randomTransformMultiplier, scale);
		auto transformId = static_cast<uint32_t>(pResources->transforms.GetSize());
        pResources->transforms.PushBack(transform);
        pResources->CreateRenderObject(transformId, pResources->meshes[meshId].firstSurface);
    }

    // Creates the rendering stress test scene. 
    // TODO: This function is unsafe, calling it after another function that creates render object will cause issues
    void LoadGeometryStressTest(RenderingResources* pResources, float transformMultiplier)
    {
        // Don't load the stress test if ray tracing is on
        #if defined(BLIT_VK_RAYTRACING)// The name of this macro should change
            return;
        #endif
        
        constexpr uint32_t bunnyCount = 2'500'000;
        constexpr uint32_t kittenCount = 1'500'000;
        constexpr uint32_t maleCount = 90'000;
		constexpr uint32_t dragonCount = 10'000;
		constexpr uint32_t totalCount = bunnyCount + kittenCount + maleCount + dragonCount;

        BLIT_WARN("Loading Renderer Stress test with %i objects", totalCount);

		uint32_t start = pResources->renderObjectCount;

        // Bunnies
        for(uint32_t i = start; i < start + bunnyCount; ++i)
        {
			CreateRenderObjectWithRandomTransform(0, pResources, transformMultiplier, 5.f);
        }
        start += bunnyCount;
        // Kittens
        for (uint32_t i = start; i < start + kittenCount; ++i)
        {
            CreateRenderObjectWithRandomTransform(2, pResources, transformMultiplier, 1.f);
        }
        // Standford dragons
        start += kittenCount;
        for (uint32_t i = start; i < start + dragonCount; ++i)
        {
			CreateRenderObjectWithRandomTransform(1, pResources, transformMultiplier, 0.5f);
        }
        // Humans
        start += dragonCount;
        for (uint32_t i = start; i < start + maleCount; ++i)
        {
			CreateRenderObjectWithRandomTransform(3, pResources, transformMultiplier, 0.2f);
        }
    }

    // Creates a scene for oblique Near-Plane clipping testing. Pretty lackluster for the time being
    void CreateObliqueNearPlaneClippingTestObject(RenderingResources* pResources)
    {
        MeshTransform transform;
        transform.pos = BlitML::vec3(30.f, 50.f, 50.f);
        transform.scale = 2.f;
        transform.orientation = BlitML::QuatFromAngleAxis(BlitML::vec3(0), 0, 0);
        pResources->transforms.PushBack(transform);

        auto& currentObject = pResources->onpcReflectiveRenderObjects[pResources->onpcReflectiveRenderObjectCount++];
        currentObject.surfaceId = pResources->meshes[3].firstSurface;
        currentObject.transformId = static_cast<uint32_t>(pResources->transforms.GetSize() - 1);

        const uint32_t nonReflectiveDrawCount = 1000;
        const uint32_t start = pResources->renderObjectCount;
        
        uint32_t meshId = pResources->meshMap["kitten"].meshId;
        for (size_t i = start; i < start + nonReflectiveDrawCount; ++i)
        {
            CreateRenderObjectWithRandomTransform(meshId, pResources, 100.f, 1.f);
        }
    }
}