#pragma once
#include "BlitCL/blitPfn.h"
#include "Renderer/Entities/Residents/RenderObject/blitMovingResident.h"
#include "Renderer/View/blitCamera.h"
#include "Renderer/Entities/Residents/blitWV.h"


namespace BlitzenEngine
{
    using ENTITY_CREATION_FLAGS = int64_t;

    using WVTICK_blitpfn = BlitCL::Pfn<void, void*, float>;

    constexpr ENTITY_CREATION_FLAGS ENTITY_CREATE_GAME_LOGIC_UPDATE = 100;
    constexpr ENTITY_CREATION_FLAGS ENTITY_CREATE_DYNAMIC_TRANSFORM = 200;

    class ComponentSystem
    {
    public:

        WV* m_pTickingWorldVariables[BlitzenCore::Ce_MaxTickingWorldVariables]{nullptr};
        uint32_t tickingWorldVariableCount{ 0 };
        
        MovingResident m_movingResidents[BlitzenCore::Ce_MaxWorldMovingResidentCount]{};
        uint32_t m_movingResidentCount{ 0 };

        // Camera
        Camera m_camera;

        void AddMovingWorldVariable();
    };
}