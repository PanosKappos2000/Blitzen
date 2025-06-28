#include "blitComponents.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
    inline ComponentSystem* pComponents_STATIC_ACCESS{ nullptr };

    void ComponentSystem::UpdateCPUTransforms()
    {
        // New transform

        // Place in Collision Grid
    }

    COLLISION_CREATE_RES RequestBlockingCollision(uint32_t movingResidentID)
    {
        auto& collisions{ pComponents_STATIC_ACCESS };

        return COLLISION_CREATE_RES::SUCCESS;
    }

    void AddMovingResident_STATIC_ACCESS(MovingResident* pMoving)
    {
        //pComponents_STATIC_ACCESS->m_movingResidents[pComponents_STATIC_ACCESS->m_movingResidentCount++] = pMoving;
    }

    void InitializeComponentSystemPointer_STATIC_ACCESS(ComponentSystem* ptr)
    {
        BLIT_ASSERT_MESSAGE(pComponents_STATIC_ACCESS == nullptr, "Attempted to reinitialize the static access component system pointer");

        pComponents_STATIC_ACCESS = ptr;
    }
}