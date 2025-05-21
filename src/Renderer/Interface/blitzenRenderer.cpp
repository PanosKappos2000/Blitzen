#define CGLTF_IMPLEMENTATION
#include "Renderer/blitRenderer.h"

namespace BlitzenEngine
{
#if defined(_WIN32)
    void UpdateRendererTransform(BlitzenDX12::Dx12Renderer* pDx12, RendererTransformUpdateContext& context)
    {
		pDx12->UpdateObjectTransform(context);
    }

    void UpdateRendererTransform(BlitzenGL::OpenglRenderer* pGL, RendererTransformUpdateContext& context)
    {
		pGL->UpdateObjectTransform(context);
    }
#endif
    void UpdateRendererTransform(BlitzenVulkan::VulkanRenderer* pVK, RendererTransformUpdateContext& context)
    {
		pVK->UpdateObjectTransform(context);
    }

    bool LoadTextureFromFile(BlitzenEngine::RenderingResources* pResources, const char* filename, const char* texName, RendererPtrType pRenderer)
    {
		auto textureCount = pResources->GetTextureCount();
		auto textures{ pResources->GetTextureArrayPointer() };

        // Don't go over the texture limit, might want to throw a warning here
        if (textureCount >= BlitzenCore::Ce_MaxTextureCount)
        {
            BLIT_WARN("Max texture count: %i, has been reached", BlitzenCore::Ce_MaxTextureCount);
            BLIT_ERROR("Texture from file: %s, failed to load", filename);
            return 0;
        }

        auto& texture = textures[textureCount];
        // If texture upload to the renderer succeeds, the texture count is incremented and the function returns successfully
        if (pRenderer->UploadTexture(texture.pTextureData, filename))
        {
            pResources->IncrementTextureCount();
            return true;
        }
        else
        {
            BLIT_ERROR("Texture from file: %s, failed to load", filename);
            return false;
        }
    }

    void LoadGltfTextures(BlitzenEngine::RenderingResources* pResources, const cgltf_data* pGltfData, uint32_t previousTextureCount, const char* gltfFilepath, RendererPtrType pRenderer)
    {
        for (size_t i = 0; i < pGltfData->textures_count; ++i)
        {
            auto pTexture = &pGltfData->textures[i];
            if (!pTexture->image)
            {
                break;
            }

            auto pImage = pTexture->image;
            if (!pImage->uri)
            {
                break;
            }

            std::string ipath = gltfFilepath;
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

            auto path = ipath + uri;

            LoadTextureFromFile(pResources, path.c_str(), path.c_str(), pRenderer);
        }
    }

    bool LoadGltfScene(BlitzenEngine::RenderingResources* pResources, const char* path, RendererPtrType pRenderer)
    {
        if (pResources->renderObjectCount >= BlitzenCore::Ce_MaxRenderObjects)
        {
            BLIT_WARN("BLITZEN_MAX_DRAW_OBJECT already reached, no more geometry can be loaded. GLTF LOADING FAILED!");
            return false;
        }

        cgltf_options options{};

        cgltf_data* pData{ nullptr };

        auto res = cgltf_parse_file(&options, path, &pData);
        if (res != cgltf_result_success)
        {
            BLIT_WARN("Failed to load gltf file: %s", path)
                return false;
        }

        // Automatic free struct
        struct CgltfScope
        {
            cgltf_data* pData;
            inline ~CgltfScope() { cgltf_free(pData); }
        };
        CgltfScope cgltfScope{ pData };

        res = cgltf_load_buffers(&options, pData, path);
        if (res != cgltf_result_success)
        {
            return false;
        }


        res = cgltf_validate(pData);
        if (res != cgltf_result_success)
        {
            return false;
        }

        BLIT_INFO("Loading GLTF scene from file: %s", path);

        // Textures
        auto previousTextureCount = pResources->GetTextureCount();
        BLIT_INFO("Loading textures");
        LoadGltfTextures(pResources, pData, previousTextureCount, path, pRenderer);

        // Materials
        BLIT_INFO("Loading materials");
        auto previousMaterialCount = pResources->GetMaterialCount();
        pResources->LoadGltfMaterials(pData, previousMaterialCount, previousTextureCount);

        // Meshes and primitives
        BlitCL::DynamicArray<uint32_t> surfaceIndices(pData->meshes_count);
		pResources->AddMeshesFromGltf(pData, path, previousMaterialCount, surfaceIndices);

        // Render objects
        BLIT_INFO("Loading scene nodes");
        pResources->CreateRenderObjectsFromGltffNodes(pData, surfaceIndices);

        return true;
    }
}