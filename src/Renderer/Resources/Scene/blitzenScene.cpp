#include "blitScene.h"

namespace BlitzenEngine
{
    void LoadGltfMeshes(MeshResources& meshContext, Material* pMaterials, const cgltf_data* pGltfData, const char* path, uint32_t previousMaterialCount, 
        BlitCL::DynamicArray<uint32_t>& surfaceIndices)
    {
        BLIT_INFO("Loading meshes and primitives");

        for (size_t i = 0; i < pGltfData->meshes_count; ++i)
        {
            const cgltf_mesh& gltfMesh = pGltfData->meshes[i];

            auto firstSurface = static_cast<uint32_t>(meshContext.m_surfaces.GetSize());

            auto& blitzenMesh = meshContext.m_meshes[meshContext.m_meshCount];
            blitzenMesh.firstSurface = firstSurface;
            blitzenMesh.surfaceCount = uint32_t(gltfMesh.primitives_count);
            blitzenMesh.meshId = uint32_t(meshContext.m_meshCount);
            meshContext.m_meshCount++;

            surfaceIndices[i] = firstSurface;

            LoadGltfMeshPrimitives(meshContext, pMaterials, pGltfData, gltfMesh, previousMaterialCount);
        }
    }

    void LoadGltfMeshPrimitives(MeshResources& meshContext, Material* pMaterials, const cgltf_data* pGltfData, const cgltf_mesh& gltfMesh, uint32_t previousMaterialCount)
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

            GenerateSurface(meshContext, vertices, indices);

            // Get the material index and pass it to the surface if there is material index
            if (prim.material)
            {
                meshContext.m_surfaces.Back().materialId = pMaterials[previousMaterialCount + cgltf_material_index(pGltfData, prim.material)].materialId;

                if (prim.material->alpha_mode != cgltf_alpha_mode_opaque)
                {
                    meshContext.m_bTransparencyList[meshContext.m_surfaces.GetSize() - 1].isTransparent = true;
                }
            }
        }
    }

    void LoadGltfNodes(RenderContainer& renders, MeshResources& meshes, cgltf_data* pGltfData, const BlitCL::DynamicArray<uint32_t>& surfaceIndices)
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
                auto transformId = uint32_t(renders.m_transforms.GetSize());
                for (size_t j = 0; j < node->mesh->primitives_count; ++j)
                {
                    CreateRenderObject(renders, meshes, transformId, surfaceOffset + static_cast<uint32_t>(j));
                }
                renders.m_transforms.PushBack(transform);
            }
        }
    }

    // THIS IS GARBAGE THAT NEEDS TO BE CLEANSED (DefineMaterial function is a good place to start)
    void LoadGltfMaterials(RenderingResources* pResources, const cgltf_data* pGltfData, uint32_t previousMaterialCount, uint32_t previousTextureCount)
    {
        for (size_t i = 0; i < pGltfData->materials_count; ++i)
        {
            auto& cgltf_mat = pGltfData->materials[i];

            auto& mat = const_cast<Material&>(pResources->GetMaterialArrayPointer()[pResources->GetMaterialCount()]);
            auto materialCount = pResources->IncrementMaterialCount();
            mat.materialId = materialCount - 1;

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
}