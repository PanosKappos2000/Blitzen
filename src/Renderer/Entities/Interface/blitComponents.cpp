#include "blitComponents.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
    inline ComponentSystem* pComponents_STATIC_ACCESS{ nullptr };

    void InitializeComponentSystemPointer_STATIC_ACCESS(ComponentSystem* ptr)
    {
        BLIT_ASSERT_MESSAGE(pComponents_STATIC_ACCESS == nullptr, "Attempted to reinitialize the static access component system pointer");

        pComponents_STATIC_ACCESS = ptr;
    }
}