#pragma once
#include "Renderer/Entities/Residents/blitResidentManager.h"
#include "Renderer/Interface/blitRenderer.h"
#include "Renderer/Resources/blitRenderingResources.h"

namespace BlitzenEngine
{
    enum class SceneType
    {
        RendererStressTest = 0,
        MovingResidentTest = 1,
        GltfSceneTest = 2
    };

    struct SCENE_CREATE_CONTEXT
    {
        SceneType m_type;
        const char* m_name;

        RendererPtrType pRenderer{ nullptr };
        WORLD_RESIDENTS* pResidents;
        RenderingResources* pResources{ nullptr };
    };

    enum class SCENE_CREATE_RES : int32_t
    {
        SUCCESS = 0,
        FATAL = BlitzenCore::CE_BLITZEN_FATAL,

        UNKNOWN = -1,
        NON_DDS_TEXTURE_FOUND = -2,
        FAILED_TO_LOAD_TEXTURE = -3,
        FAILED_TO_LOAD_GLTF_FILE = -4,
        FAILED_TO_MODIFY_TEXTURE_FILEPATH_TO_DDS = -5,
        FAILED_TO_LOAD_TEXTURE_TO_GPU = -6,
        FAILED_TO_ADD_TEXTURE_TO_SYSTEM = -7,
        MESH_LOADING_FAILED = -8,

        INVALID_RENDERER_HANDLE = - 5000,
        INVALID_WORLD_RESIDENTS_HANDLE = - 10000,
        INVALID_RENDERING_RESOURCES_HANDLE = -1'000,

        SCENE_RESIDENTS_FAILURE = -2000
    };

    inline const char* GET_SCENE_CREATE_RES_STRING(SCENE_CREATE_RES res)
    {
        switch (res)
        {
        case SCENE_CREATE_RES::NON_DDS_TEXTURE_FOUND: return "NON_DDS_TEXTURE_FOUND";
        case SCENE_CREATE_RES::UNKNOWN: default: return "UNKNOW RES";
        case SCENE_CREATE_RES::SUCCESS: return "SUCCESS";
        case SCENE_CREATE_RES::FAILED_TO_LOAD_TEXTURE: return "FAILED_TO_LOAD_TEXTURE";
        case SCENE_CREATE_RES::FAILED_TO_LOAD_GLTF_FILE: return "FAILED_TO_LOAD_GLTF_FILE";
        case SCENE_CREATE_RES::FAILED_TO_MODIFY_TEXTURE_FILEPATH_TO_DDS: return "FAILED_TO_MODIFY_TEXTURE_FILEPATH_TO_DDS";
        case SCENE_CREATE_RES::FAILED_TO_LOAD_TEXTURE_TO_GPU: return "FAILED_TO_LOAD_TEXTURE_TO_GPU";
        case SCENE_CREATE_RES::FAILED_TO_ADD_TEXTURE_TO_SYSTEM: return "FAILED_TO_ADD_TEXTURE_TO_SYSTEM";
        case SCENE_CREATE_RES::SCENE_RESIDENTS_FAILURE: return "SCENE_RESIDENTS_FAILURE";
        case SCENE_CREATE_RES::MESH_LOADING_FAILED: return "MESH_LOADING_FAILED";
        case SCENE_CREATE_RES::INVALID_RENDERER_HANDLE: return "INVALID_RENDERER_HANDLE";
        case SCENE_CREATE_RES::INVALID_WORLD_RESIDENTS_HANDLE: return "INVALID_WORLD_RESIDENTS_HANDLE";
        case SCENE_CREATE_RES::INVALID_RENDERING_RESOURCES_HANDLE: return "INVALID_RENDERING_RESOURCES_HANDLE";
        }
    }

    struct SceneContext
    {
        Mesh* m_meshRefArr[BlitzenCore::ARRAY_SIZE_PLACEHOLDER]{nullptr};

        RenderObject* m_renderRefArr[BlitzenCore::ARRAY_SIZE_PLACEHOLDER]{nullptr};

        RenderObject* m_transparentArr[BlitzenCore::ARRAY_SIZE_PLACEHOLDER]{ nullptr };
    };

    struct SceneContainer
    {
        bool empty = true;
    };

    SCENE_CREATE_RES CreateScene(SceneContext* pScene, SCENE_CREATE_CONTEXT& scene);

    SCENE_CREATE_RES LoadGeometryStressTest(WORLD_RESIDENTS* pResidents, BlitzenEngine::RenderingResources* pResources, float transformMultiplier, BlitzenEngine::SceneContext* pScene);

    SCENE_CREATE_RES LoadMovingResidentTest(WORLD_RESIDENTS* pResidents, float transformMultiplier);
}