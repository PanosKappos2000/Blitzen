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

        
        
    };

    void AddMovingResident_STATIC_ACCESS(MovingResident* pMoving);

    void InitializeComponentSystemPointer_STATIC_ACCESS(ComponentSystem* ptr);
}