#define CGLTF_IMPLEMENTATION
#include "gltfScene.h"
#include "BlitCL/blitDynamicArr.h"
#include "Core/DbLog/blitLogger.h"
#include "Renderer/World/blitWorldReq.h"
#include "Renderer/World/blitzenWorld.h"

namespace BlitzenEngine
{
    CgltfScope::~CgltfScope()
    {
        if (pData != nullptr)
        {
            cgltf_free(pData);
        }

        if (renderObjects != nullptr)
        {
            BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Entity, renderObjects, residentCount * sizeof(RenderObject));
        }

        if (meshTransforms != nullptr)
        {
            BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Entity, meshTransforms, residentCount * sizeof(MeshTransform));
        }
    }

    SCENE_CREATE_RES ManageGltf(const char* filepath, const char* sceneName, RenderingResources* pResources, WORLD_RESIDENTS* pWorldResidents, RendererPtrType pRenderer)
    {
        auto& textureContext{ pResources->m_textureManager };
        auto& meshContext{ pResources->m_meshContext };

        CgltfScope cgltfScope;
        cgltfScope.pData = nullptr;
        cgltfScope.sceneName = sceneName;

        if (!LoadGltfFile(filepath, cgltfScope))
        {
            return SCENE_CREATE_RES::FAILED_TO_LOAD_GLTF_FILE;
        }

        // Texture count saved for materials
        uint32_t previousTextureCount = textureContext.m_textureCount;

        // Textures (special care because they are directly managed by the renderer backend)
        BLIT_INFO("Loading textures for GLTF");
        for (size_t i = 0; i < cgltfScope.pData->textures_count; ++i)
        {
            // Change to dds texture
            auto pTexture = &cgltfScope.pData->textures[i];
            std::string ddsFilepath{ "" };
            if (!BlitzenEngine::ModifyTextureFilepath(pTexture, filepath, ddsFilepath))
            {
                return SCENE_CREATE_RES::FAILED_TO_MODIFY_TEXTURE_FILEPATH_TO_DDS;
            }

            // Give to renderer
            if (!UploadTextureToGPU(pRenderer, textureContext.m_singleTextureHandle, ddsFilepath.c_str()))
            {
                return SCENE_CREATE_RES::FAILED_TO_LOAD_TEXTURE_TO_GPU;
            }

            // Update textures (might want to return if this fails)
            if (!textureContext.AddTexture(ddsFilepath.c_str()))
            {
                return SCENE_CREATE_RES::FAILED_TO_ADD_TEXTURE_TO_SYSTEM;
            }
        }

        // Materials
        BLIT_INFO("Loading materials for GLTF");
        LoadGltfMaterials(textureContext, cgltfScope, previousTextureCount);

        // Meshes
        BLIT_INFO("Loading meshes for GLTF");
        if (!LoadGltfMeshes(meshContext, textureContext, cgltfScope, pResources->mLoadingContextMesh))
        {
            return SCENE_CREATE_RES::MESH_LOADING_FAILED;
        }

        BLIT_INFO("Loading scene nodes");
        if (!LoadGltfNodes(pWorldResidents, meshContext, cgltfScope))
        {
            return SCENE_CREATE_RES::SCENE_RESIDENTS_FAILURE;
        }

        // success
        return SCENE_CREATE_RES::SUCCESS;
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

    bool LoadGltfMeshes(MeshResources& meshContext, TextureManager& textureContext, CgltfScope& cgltfScope, RenderingLoadingContextMesh& loadingContextMesh)
    {
        if (meshContext.m_meshPrimitives.m_meshPrimitivesCount != 0)
        {
            BLIT_ERROR("%s: Cannot load gltf meshes, as the mesh context was not reset before gltf loading", BlitzenCore::CE_SCENE_SYSTEM_NAME);
            return false;
        }

        // Creates scene folder for resources
        BlitCL::String sceneFilepath{ BLITZEN_CLIENT_RPFMESH_DIRECTORY };
        sceneFilepath.Append(const_cast<char*>(cgltfScope.sceneName));
        if (!BlitzenPlatform::CreateDirectoryIfMissing(sceneFilepath.GetClassic()))
        {
            BLIT_ERROR("%s: Failed to create scene directory", BlitzenCore::CE_SCENE_SYSTEM_NAME);
            return false;
        }

        for (size_t m = 0; m < cgltfScope.pData->meshes_count; ++m)
        {
            for (size_t s = 0; s < cgltfScope.pData->meshes[m].primitives_count; ++s) cgltfScope.meshPrimitiveCount++;
        }

        for (size_t m = 0; m < cgltfScope.pData->meshes_count; ++m)
        {
            const auto& gltfMesh = cgltfScope.pData->meshes[m];

            if (!LoadGltfMeshPrimitives(meshContext, textureContext, cgltfScope, gltfMesh, loadingContextMesh, (uint32_t)m))
            {
				BLIT_ERROR("%s: Failed to load mesh primitive for mesh number: (%u)", BlitzenCore::CE_SCENE_SYSTEM_NAME);
                return false;
            }
        }

        return true;
    }

    bool LoadGltfMeshPrimitives(MeshResources& meshContext, TextureManager& textureContext, CgltfScope& cgltfScope, const cgltf_mesh& gltfMesh,
        RenderingLoadingContextMesh& loadingContextMesh, uint32_t meshID)
    {
        for (size_t j = 0; j < gltfMesh.primitives_count; ++j)
        {
            const cgltf_primitive& prim = gltfMesh.primitives[j];

            // Skips primitives that do not consist of triangles
            if (prim.type != cgltf_primitive_type_triangles || !prim.indices)
            {
                BLIT_ERROR("Blitzen supports only primitives with cgltf_primitive_type_triangles flags set and with indices");
                return false;
            }

            size_t vertexCount = prim.attributes[0].data->count;
            BlitCL::DynamicArray<Vertex> vertices{ vertexCount };
            BlitCL::DynamicArray<float> scratch{ vertexCount * 4 };

            // Vertex positions
            if (const cgltf_accessor* pos = cgltf_find_accessor(&prim, cgltf_attribute_type_position, 0))
            {
                if (cgltf_num_components(pos->type) != 3)
                {
                    BLIT_ERROR("Found gltf pos component with count not equal to 3");
                    return false;
                }

                cgltf_accessor_unpack_floats(pos, scratch.Data(), vertexCount * 3);
                for (size_t j = 0; j < vertexCount; ++j)
                {
                    vertices[j].position = BlitML::vec3(scratch[j * 3 + 0], scratch[j * 3 + 1], scratch[j * 3 + 2]);
                }
            }

            // Vertex normals
            if (const cgltf_accessor* nrm = cgltf_find_accessor(&prim, cgltf_attribute_type_normal, 0))
            {
                if (cgltf_num_components(nrm->type) != 3)
                {
                    BLIT_ERROR("Found gltf normal component with count not equal to 3");
                    return false;
                }

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
                if (cgltf_num_components(tang->type) != 4)
                {
                    BLIT_ERROR("Found gltf tangent component with count not equal to 4");
                    return false;
                }

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
                if (cgltf_num_components(tex->type) != 2)
                {
                    BLIT_ERROR("Found gltf tex coord component with count not equal to 2");
                    return false;
                }

                cgltf_accessor_unpack_floats(tex, scratch.Data(), vertexCount * 2);
                for (size_t j = 0; j < vertexCount; ++j)
                {
                    vertices[j].uvX = scratch[j * 2 + 0];
                    vertices[j].uvY = scratch[j * 2 + 1];
                }
            }

            BlitCL::DynamicArray<uint32_t> indices(prim.indices->count);
            cgltf_accessor_unpack_indices(prim.indices, indices.Data(), 4, indices.GetSize());

            // SURFACE GENERATION WITH LOADED INDICES
            MESH_PRIMITIVE_CREATE_CONTEXT meshPrimitiveContext{};
            meshPrimitiveContext.m_indexCount = uint32_t(indices.GetSize());
            meshPrimitiveContext.m_indices = indices.Data();
            meshPrimitiveContext.m_vertexCount = uint32_t(vertices.GetSize());
            meshPrimitiveContext.m_vertices = vertices.Data();
            if (prim.material)
            {
                if (prim.material->alpha_mode != cgltf_alpha_mode_opaque)
                {
                    meshPrimitiveContext.m_specialFlags |= MESH_PRIMITIVE_SPECIAL_TRANSPARENT;
                }
                meshPrimitiveContext.m_materialID = 0;//textureContext.m_materials[previousMaterialCount + cgltf_material_index(cgltfScope.pData, prim.material)].materialId;
            }

            auto meshPrimitiveRes{ meshContext.m_meshPrimitives.GenerateSurface(meshContext.m_triangles, meshContext.m_clusters, meshPrimitiveContext) };
            if (BlitzenCore::BLIT_CHECK_FAIL(int64_t(meshPrimitiveRes)))
            {
                return BlitzenCore::LOG_ERROR_MSG_AND_RETURN(BlitzenCore::CE_SCENE_SYSTEM_NAME, MESH_PRIMITIVE_CREATE_RES_TO_STRING(meshPrimitiveRes));
            }

            uint32_t meshNum = meshID + (uint32_t)j;
            BlitCL::String meshPrimitiveEndPath{ cgltfScope.sceneName };
            meshPrimitiveEndPath.Append("/mesh");
            char meshNameSuffix[16]; 
            snprintf(meshNameSuffix, sizeof(meshNameSuffix), "%u", meshNum);
            meshPrimitiveEndPath.Append(meshNameSuffix);
            
            BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE memoryMappedFile;
            auto uploadRes{ UploadMeshToDisk(meshPrimitiveEndPath.GetClassic(), memoryMappedFile, meshContext, false)};
            if (BlitzenCore::BLIT_CHECK_FAIL(int64_t(uploadRes)))
            {
                BLIT_ERROR("%s: Failed to upload mesh resource to disk. Received error: %s", BlitzenCore::CE_MESH_SYSTEM_NAME, UPLOAD_MESH_TO_DISK_RES_TO_STRING(uploadRes));
                return false;
            }
            meshContext.ResetMeshContext();
        }

        return true;
    }

    static void DecomposeTransform(float translation[3], float rotation[4], float scale[3], const float* transform)
    {
        // I could be using my own matrix type but this function is copied from elsewhere and it would be a pain trying to convert it
        // TODO: Try to implement this differently to fit the engine
        float m[4][4] = {};
        BlitzenCore::BlitMemCopy(m, (void*)transform, 16 * sizeof(float));

        // Extract translation from last row
        translation[0] = m[3][0];
        translation[1] = m[3][1];
        translation[2] = m[3][2];

        // Compute determinant to determine handedness
        float det =
            m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2]) -
            m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
            m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
        float sign = (det < 0.f) ? -1.f : 1.f;

        // Recover scale from axis lengths
        scale[0] = sqrtf(m[0][0] * m[0][0] + m[0][1] * m[0][1] + m[0][2] * m[0][2]) * sign;
        scale[1] = sqrtf(m[1][0] * m[1][0] + m[1][1] * m[1][1] + m[1][2] * m[1][2]) * sign;
        scale[2] = sqrtf(m[2][0] * m[2][0] + m[2][1] * m[2][1] + m[2][2] * m[2][2]) * sign;

        // Normalize axes to get a pure rotation matrix
        float rsx = (scale[0] == 0.f) ? 0.f : 1.f / scale[0];
        float rsy = (scale[1] == 0.f) ? 0.f : 1.f / scale[1];
        float rsz = (scale[2] == 0.f) ? 0.f : 1.f / scale[2];
        float r00 = m[0][0] * rsx, r10 = m[1][0] * rsy, r20 = m[2][0] * rsz;
        float r01 = m[0][1] * rsx, r11 = m[1][1] * rsy, r21 = m[2][1] * rsz;
        float r02 = m[0][2] * rsx, r12 = m[1][2] * rsy, r22 = m[2][2] * rsz;

        // "branchless" version of Mike Day's matrix to quaternion conversion
        int qc = r22 < 0 ? (r00 > r11 ? 0 : 1) : (r00 < -r11 ? 2 : 3);
        float qs1 = qc & 2 ? -1.f : 1.f;
        float qs2 = qc & 1 ? -1.f : 1.f;
        float qs3 = (qc - 1) & 2 ? -1.f : 1.f;
        float qt = 1.f - qs3 * r00 - qs2 * r11 - qs1 * r22;
        float qs = 0.5f / sqrtf(qt);
        rotation[qc ^ 0] = qs * qt;
        rotation[qc ^ 1] = qs * (r01 + qs1 * r10);
        rotation[qc ^ 2] = qs * (r20 + qs2 * r02);
        rotation[qc ^ 3] = qs * (r12 + qs3 * r21);
    }

    bool LoadGltfNodes(WORLD_RESIDENTS* pResidents, MeshResources& meshContext, CgltfScope& cgltfScope)
    {
        for (size_t i = 0; i < cgltfScope.pData->nodes_count; ++i)
        {
            auto node = &cgltfScope.pData->nodes[i];
            if (node->mesh)
            {
                uint32_t meshIdx = (uint32_t)cgltf_mesh_index(cgltfScope.pData, node->mesh);
                uint32_t meshPrimitiveCount = (uint32_t)cgltfScope.pData->meshes[meshIdx].primitives_count;
                for (uint32_t primID = 0; primID < meshPrimitiveCount; ++primID) cgltfScope.residentCount++;
            }
        }

        cgltfScope.renderObjects = reinterpret_cast<RenderObject*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Entity, cgltfScope.residentCount * sizeof(RenderObject)));
        cgltfScope.meshTransforms = reinterpret_cast<MeshTransform*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Entity, cgltfScope.residentCount * sizeof(MeshTransform)));
        
        uint32_t residentID = 0;
        for (size_t i = 0; i < cgltfScope.pData->nodes_count; ++i)
        {
            auto node = &cgltfScope.pData->nodes[i];

            // Create render objects for mesh nodes
            if (node->mesh)
            {
                // Gets the model matrix
                float matrix[16];
                cgltf_node_transform_world(node, matrix);

                // Switches to split world transform by decomposing the matrix.
                float translation[3];
                float rotation[4];
                float scale[3];
                DecomposeTransform(translation, rotation, scale, matrix);

                MeshTransform transform;
                transform.pos = BlitML::vec3(translation[0], translation[1], translation[2]);
                transform.scale = BlitML::FMax(scale[0], BlitML::FMax(scale[1], scale[2]));
                transform.orientation = BlitML::quat(rotation[0], rotation[1], rotation[2], rotation[3]);

                // TODO: better warnings for non-uniform or negative scale

                // Gets id from surface indices
                uint32_t meshIdx = (uint32_t)cgltf_mesh_index(cgltfScope.pData, node->mesh);
                uint32_t meshPrimitiveCount = (uint32_t)cgltfScope.pData->meshes[meshIdx].primitives_count;
                
                for (uint32_t primID = 0; primID < meshPrimitiveCount; ++primID)
                {
                    cgltfScope.renderObjects[residentID].surfaceId = meshIdx + primID;
                    cgltfScope.renderObjects[residentID].transformId = residentID;
                    cgltfScope.meshTransforms[residentID] = transform;

                    residentID++;
                }
            }
        }

        if (residentID != cgltfScope.residentCount) return false;

        if (!UploadImportedSceneNodesToDisk(cgltfScope.sceneName, cgltfScope.meshPrimitiveCount, cgltfScope.residentCount, cgltfScope.renderObjects, cgltfScope.meshTransforms))
        {
            BLIT_ERROR("%s: Failed to upload gltf nodes to rpf", BlitzenCore::CE_SCENE_SYSTEM_NAME);
            return false;
        }

        // success
        return true;
    }

    void LoadGltfMaterials(TextureManager& textureContext, CgltfScope& cgltfScope, uint32_t previousTextureCount)
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

            if (!textureContext.AddMaterial(albedoId, normalId, specularId, emissiveId))
            {
                BLIT_ERROR("Failed to add GLTF material number: (%u)", i);
                break;
            }
        }
    }
}