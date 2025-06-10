#pragma once
#include "Renderer/View/blitCamera.h"
#include "Core/Events/blitTimeManager.h"
#include "Renderer/Entities/Interface/blitEntityManager.h"
#include "BlitCL/blitPfn.h"

namespace BlitzenWorld
{
    using WORLD_VARIABLE_CREATE_FLAGS = uint32_t;
    constexpr WORLD_VARIABLE_CREATE_FLAGS WV_TICKING_BIT = 0x5;
    constexpr WORLD_VARIABLE_CREATE_FLAGS WV_DYNAMIC_TRANSFORM_BIT = 0xA;

    struct WV_CREATE_CONTEXT
    {
        void* m_DATA{ nullptr };
        size_t m_dataSize{ 0 };
        uint32_t m_resId{ 0 };

        WORLD_VARIABLE_CREATE_FLAGS m_flags{ 0 };

        WVTICK_blitpfn m_PFNTICK{};

        BlitzenEngine::Mesh* m_pMesh{ nullptr };
        BlitzenEngine::MeshTransform* m_pInitialTransform{ nullptr };

        BlitzenEngine::DynamicTransform* m_pDynamicTranform{ nullptr };
    };

    using WVTICK_blitpfn = BlitCL::Pfn<void, void*, float>;

    struct WorldVariable
    {
        void* pWVDATA{ nullptr };

        WVTICK_blitpfn PFNTICK{};
    };

    enum class WorldUpdateState : uint8_t
    {
        PREPARING = 0,
        READY = 1,
        DISPATCHED = 2,
        WAITING = 3,
    };

    enum class WorldUpdateType : int32_t
    {
        NO_UPDATE = 0,

        ADD_DYNAMIC_TRANSFORM = 1,
        REMOVE_DYNAMIC_TRANSFORM = 2,
        CHANGE_DYNAMIC_TRANSFORM = 3,
        BLOCK_DYNAMIC_TRANSFORM = 4
    };
    constexpr uint32_t CE_WORLD_UPDATE_RECEIVER_MAX_WORK_COUNT = 10;

    struct BlitzenWorldUpdate
    {
        WorldUpdateType m_worldUpdateFlags[CE_WORLD_UPDATE_RECEIVER_MAX_WORK_COUNT]{ WorldUpdateType::NO_UPDATE };
        uint32_t m_updateIDX[CE_WORLD_UPDATE_RECEIVER_MAX_WORK_COUNT]{ 0 };
        uint8_t m_worldUpdateWorkCount{ 0 };
        WorldUpdateState m_state{ WorldUpdateState::WAITING };
    };

    struct BlitzenWorldContext
    {
        BlitzenEngine::CameraContainer* pCameraContainer;

        WorldVariable WVs[BlitzenCore::Ce_MaxWorldVariableCount];
        uint32_t wvCount{ 0 };

        BlitzenWorldUpdate worldUpdates[BlitzenCore::Ce_MaxWorldVariableCount];
        uint32_t worldUpdateCount = 0;
        
        float deltaTime;
    };

    template<class WVDATA>
    void AddWorldVariable(BlitzenWorldContext& WORLD, WV_CREATE_CONTEXT& context)
    {
        BLIT_ASSERT_MESSAGE(WORLD.wvCount < BlitzenCore::Ce_MaxWorldVariableCount, "Exceeded allowed world variable count");
        BLIT_ASSERT_MESSAGE(context.m_dataSize != 0, "Can create world variable with data size 0");

        WORLD.worldUpdateCount++;
        auto& WV{ WORLD.WVs[WORLD.wvCount++] };

        WV.pWVDATA = BlitzenCore::BlitAlloc<uint8_t>(BlitzenCore::AllocationType::Entity, sizeof(WVDATA);

        if (context.flags & WV_TICKING_BIT)
        {
            BLIT_ASSERT_MESSAGE(context.m_PFNTICK.IsFunctional(), "Passed ticking flag without ticking behaviour function pointer");
            WV.PFNTICK = context.m_PFNTICK;
        }

        if (context.flags & WV_RENDER_BIT)
        {
            BlitzenEngine::CreateRenderObjectFromMesh()
        }

        reinterpret_cast<WVDATA*>(WV.pWVDATA)->Start(context);
    }
}