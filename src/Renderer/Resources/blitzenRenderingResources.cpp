#include "blitRenderingResources.h"
#include "Renderer/Interface/blitRenderer.h"
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
    RenderingResources::RenderingResources()
    {
        DefineMaterial(BlitML::vec4{ 0.1f }, 65.f, "dds_texture_default", "unknown", "loaded_material");
        LoadMeshFromObj(m_meshContext, "Assets/Meshes/bunny.obj", BlitzenCore::Ce_DefaultMeshName);
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

    bool RenderingResources::CreateRenderObject(uint32_t transformId, uint32_t surfaceId)
    {
        if (renderObjectCount >= BlitzenCore::Ce_MaxRenderObjects)
        {
            BLIT_ERROR("Max render object count reached");
            return false;
        }

        if (!m_meshContext.m_bTransparencyList[surfaceId].isTransparent)
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

    uint32_t RenderingResources::AddRenderObjectsFromMesh(uint32_t meshId,
        const BlitzenEngine::MeshTransform& transform, bool bDynamic)
    {
        auto& mesh = m_meshContext.m_meshes[meshId];
        if (renderObjectCount + mesh.surfaceCount >= BlitzenCore::Ce_MaxRenderObjects)
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

    void RenderingResources::AddMeshesFromGltf(const cgltf_data* pGltfData, const char* path, uint32_t previousMaterialCount, BlitCL::DynamicArray<uint32_t>& surfaceIndices)
    {
        
        BLIT_INFO("Loading meshes and primitives");
        for (size_t i = 0; i < pGltfData->meshes_count; ++i)
        {
            const cgltf_mesh& gltfMesh = pGltfData->meshes[i];

            auto firstSurface = static_cast<uint32_t>(m_meshContext.m_surfaces.GetSize());

            auto& blitzenMesh = m_meshContext.m_meshes[m_meshContext.m_meshCount];
            blitzenMesh.firstSurface = firstSurface;
            blitzenMesh.surfaceCount = uint32_t(gltfMesh.primitives_count);
            blitzenMesh.meshId = uint32_t(m_meshContext.m_meshCount);
            m_meshContext.m_meshCount++;

            surfaceIndices[i] = firstSurface;

            AddPrimitivesFromGltf(pGltfData, gltfMesh, previousMaterialCount);
        }
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

            GenerateSurface(m_meshContext, vertices, indices);

            // Get the material index and pass it to the surface if there is material index
            if (prim.material)
            {
                m_meshContext.m_surfaces.Back().materialId = m_materials[previousMaterialCount + cgltf_material_index(pGltfData, prim.material)].materialId;

                if (prim.material->alpha_mode != cgltf_alpha_mode_opaque)
                {
					m_meshContext.m_bTransparencyList[m_meshContext.m_surfaces.GetSize() - 1].isTransparent = true;
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

    void RenderingResources::LoadGltfMaterials(const cgltf_data* pGltfData, uint32_t previousMaterialCount, uint32_t previousTextureCount)
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

    void RenderingResources::CreateSingleObjectForTesting()
    {
		// Create a single object for testing
		MeshTransform transform;
		transform.pos = BlitML::vec3(BlitzenCore::Ce_InitialCameraX, BlitzenCore::Ce_initialCameraY, BlitzenCore::Ce_initialCameraZ);
		transform.scale = 10.f;
		transform.orientation = BlitML::QuatFromAngleAxis(BlitML::vec3(0), 0, 0);
        AddRenderObjectsFromMesh(m_meshContext.m_meshMap["kitten"].meshId, transform, false);
		
    }

    void RandomizeTransform(MeshTransform& transform, float multiplier, float scale)
    {
        transform.pos = BlitML::vec3((float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier, (float(rand()) / RAND_MAX) * multiplier);

        transform.scale = scale;

        transform.orientation = BlitML::QuatFromAngleAxis(BlitML::vec3((float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1, (float(rand()) / RAND_MAX) * 2 - 1),
            BlitML::Radians((float(rand()) / RAND_MAX) * 90.f), 0);
    }

    static void CreateRenderObjectWithRandomTransform(uint32_t meshId, RenderingResources* pResources, float randomTransformMultiplier, float scale)
    {
		if (pResources->renderObjectCount >= BlitzenCore::Ce_MaxRenderObjects)
		{
            BLIT_WARN("Max render object count reached");
			return;
		}

        auto& meshContext{ pResources->GetMeshContext() };
		// Get the mesh used by the current game object
		auto& currentMesh = meshContext.m_meshes[meshId];
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
        pResources->CreateRenderObject(transformId, meshContext.m_meshes[meshId].firstSurface);
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

		auto& meshContext{ pResources->GetMeshContext() };

        auto& currentObject = pResources->onpcReflectiveRenderObjects[pResources->onpcReflectiveRenderObjectCount++];
        currentObject.surfaceId = meshContext.m_meshes[3].firstSurface;
        currentObject.transformId = static_cast<uint32_t>(pResources->transforms.GetSize() - 1);

        const uint32_t nonReflectiveDrawCount = 1000;
        const uint32_t start = pResources->renderObjectCount;
        
        uint32_t meshId = meshContext.m_meshMap["kitten"].meshId;
        for (size_t i = start; i < start + nonReflectiveDrawCount; ++i)
        {
            CreateRenderObjectWithRandomTransform(meshId, pResources, 100.f, 1.f);
        }
    }
}