#pragma once
#include "BlitCL/blitPfn.h"
#include "Renderer/Entities/Residents/Dynamic/blitMovingResident.h"
#include "Renderer/View/blitCamera.h"
#include "Renderer/Entities/Residents/blitWV.h"


namespace BlitzenEngine
{

    class ComponentSystem
    {
    public:

        WVKEY m_tickingWorldVariables[BlitzenCore::Ce_MaxTickingWorldVariables]{};
        uint32_t m_tickingWorldVariableCount{ 0 };
        
        MovingResident* m_movingResidents[BlitzenCore::Ce_MaxWorldMovingResidentCount]{};
        uint32_t m_movingResidentCount{ 0 };

        // Camera
        Camera m_camera;

        void AddTickingWV(WVKEY key);

        void UpdateCPUTransforms();

        void EvaluateCollisionResults();
    };

    void AddMovingResident_STATIC_ACCESS(MovingResident* pMoving);

    void InitializeComponentSystemPointer_STATIC_ACCESS(ComponentSystem* ptr);
}