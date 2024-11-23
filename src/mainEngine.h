#include "Core/blitAssert.h"
#include "Platform/platform.h"

namespace BlitzenEngine
{
    class BlitzenEngine
    {
    public:
        BlitzenEngine();

        void Run();

        ~BlitzenEngine();

    private:

        //Used to create the window and other platform specific part of the engine
        BlitzenPlatform::PlatformState m_platformState;
    };
}
