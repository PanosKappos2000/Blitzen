#include "blitScene.h"

namespace BlitzenEngine
{
    CgltfScope::~CgltfScope()
    {
        if (pData)
        {
            cgltf_free(pData);
        }
	}

    bool LoadGltfFile(const char* path, CgltfScope& cgltf)
    {
        cgltf_options options{};

        cgltf.pData = nullptr;

        auto res = cgltf_parse_file(&options, path, &cgltf.pData);
        if (res != cgltf_result_success)
        {
			BLIT_ERROR("Failed to parse gltf file: %s", path);
            return false;
        }

        res = cgltf_load_buffers(&options, cgltf.pData, path);
        if (res != cgltf_result_success)
        {
			BLIT_ERROR("Failed to load gltf buffers: %s", path);
            return false;
        }


        res = cgltf_validate(cgltf.pData);
        if (res != cgltf_result_success)
        {
			BLIT_ERROR("Failed to validate gltf file: %s", path);
            return false;
        }

        BLIT_INFO("Loading GLTF scene from file: %s", path);
        return true;
    }

	bool ModifyTextureFilepath(cgltf_texture* pTexture, const char* fullPath, std::string& texturePath)
	{
        
        if (!pTexture->image)
        {
            BLIT_ERROR("No image resource found in gltf texture");
            return false;
        }
        auto pImage = pTexture->image;

        if (!pImage->uri)
        {
            BLIT_ERROR("gltf image has no uri");
            return false;
        }

        std::string ipath{ fullPath };
        auto pos = ipath.find_last_of('/');
        if (pos == std::string::npos)
        {
            ipath = "";
        }
        else
        {
            ipath = ipath.substr(0, pos + 1);
        }

        std::string uri{ pImage->uri };
        uri.resize(cgltf_decode_uri(&uri[0]));
        auto dot = uri.find_last_of('.');

        if (dot != std::string::npos)
        {
            uri.replace(dot, uri.size() - dot, ".dds");
        }

        texturePath = ipath + uri;
        return true;
	}

    void LoadGltfMeshes(MeshResources& meshContext, TextureManager& textureContext, const CgltfScope& cgltfScope, uint32_t previousMaterialCount, BlitCL::DynamicArray<uint32_t>& surfaceIndices)
    {
        for (size_t i = 0; i < cgltfScope.pData->meshes_count; ++i)
        {
            const auto& gltfMesh = cgltfScope.pData->meshes[i];

            auto firstSurface = uint32_t(meshContext.m_surfaces.GetSize());

            if (!meshContext.AddMesh(firstSurface, uint32_t(gltfMesh.primitives_count)))
            {
                BLIT_ERROR("Failed to add gltf mesh number: (%u)", i);
                break;
            }

            // Saves surface indices for nodes
            surfaceIndices[i] = firstSurface;

            LoadGltfMeshPrimitives(meshContext, textureContext, cgltfScope, gltfMesh, previousMaterialCount);
        }
    }

    void LoadGltfMeshPrimitives(MeshResources& meshContext, TextureManager& textureContext, const CgltfScope& cgltfScope, const cgltf_mesh& gltfMesh, uint32_t previousMaterialCount)
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
                meshContext.m_surfaces.Back().materialId = textureContext.m_materials[previousMaterialCount + cgltf_material_index(cgltfScope.pData, prim.material)].materialId;

                if (prim.material->alpha_mode != cgltf_alpha_mode_opaque)
                {
                    meshContext.m_bTransparencyList[meshContext.m_surfaces.GetSize() - 1].isTransparent = true;
                }
            }
        }
    }

    void LoadGltfNodes(RenderContainer& renders, MeshResources& meshContext, const CgltfScope& cgltfScope, const BlitCL::DynamicArray<uint32_t>& surfaceIndices)
    {
        for (size_t i = 0; i < cgltfScope.pData->nodes_count; ++i)
        {
            auto node = &cgltfScope.pData->nodes[i];

            // Create render objects for mesh nodes
            if (node->mesh)
            {
                // Gets the model matrix
                float matrix[16];
                cgltf_node_transform_world(node, matrix);

                // Switch gltf transform to Blitzen's transform
                float translation[3];
                float rotation[4];
                float scale[3];
                BlitML::decomposeTransform(translation, rotation, scale, matrix);
                MeshTransform transform;
                transform.pos = BlitML::vec3(translation[0], translation[1], translation[2]);
                transform.scale = BlitML::Max(scale[0], BlitML::Max(scale[1], scale[2]));
                transform.orientation = BlitML::quat(rotation[0], rotation[1], rotation[2], rotation[3]);

                // TODO: better warnings for non-uniform or negative scale

                // Gets id from surface indices
                auto surfaceOffset = surfaceIndices[cgltf_mesh_index(cgltfScope.pData, node->mesh)];
                auto transformId = renders.m_staticTransformOffset;
                renders.m_transforms[renders.m_staticTransformOffset++] = transform;
				renders.m_transformCount++;

                // Adds mesh primitives as render objects
                bool bPrimitivesLoaded = true;
                for (size_t primitiveId = 0; primitiveId < node->mesh->primitives_count; ++primitiveId)
                {
                    if(!CreateRenderObject(renders, meshContext, transformId, surfaceOffset + uint32_t(primitiveId)))
					{
						BLIT_ERROR("Failed to create render object for gltf node: %u", surfaceOffset + uint32_t(primitiveId));
                        bPrimitivesLoaded = false;
					}
                }

                // Checks if primitives were loaded successfully
                if (!bPrimitivesLoaded)
                {
					BLIT_ERROR("Stopped adding gltf nodes at: %u", i);
                    break;
                }
            }
        }
    }

    void LoadGltfMaterials(TextureManager& textureContext, const CgltfScope& cgltfScope, uint32_t previousTextureCount)
    {
        for (size_t i = 0; i < cgltfScope.pData->materials_count; ++i)
        {
            auto& cgltfMaterial = cgltfScope.pData->materials[i];

            uint32_t albedoId = 
                cgltfMaterial.pbr_metallic_roughness.base_color_texture.texture ? uint32_t(previousTextureCount + cgltf_texture_index(cgltfScope.pData, cgltfMaterial.pbr_metallic_roughness.base_color_texture.texture))
                : cgltfMaterial.pbr_specular_glossiness.diffuse_texture.texture ? uint32_t(previousTextureCount + cgltf_texture_index(cgltfScope.pData, cgltfMaterial.pbr_specular_glossiness.diffuse_texture.texture))
                : 0;

            uint32_t normalId = cgltfMaterial.normal_texture.texture ? uint32_t(previousTextureCount + cgltf_texture_index(cgltfScope.pData, cgltfMaterial.normal_texture.texture)) : 0;

            uint32_t specularId = 
                cgltfMaterial.pbr_specular_glossiness.specular_glossiness_texture.texture ? uint32_t(previousTextureCount + cgltf_texture_index(cgltfScope.pData, cgltfMaterial.pbr_specular_glossiness.specular_glossiness_texture.texture))
                : 0;

            uint32_t emissiveId = cgltfMaterial.emissive_texture.texture ? uint32_t(previousTextureCount + cgltf_texture_index(cgltfScope.pData, cgltfMaterial.emissive_texture.texture))
                : 0;

            if(!textureContext.AddMaterial(albedoId, normalId, specularId, emissiveId))
			{
				BLIT_ERROR("Failed to add GLTF material number: (%u)", i);
				break;
			}
        }
    }
}