#include "mainEngine.h"
#include "Platform/platform.h"

namespace BlitzenEngine
{
    // Static member variable needs to be declared
    Engine* Engine::pEngineInstance;

    Engine::Engine()
    {
        // There should not be a 2nd instance of Blitzen Engine
        if(GetEngineInstancePointer())
        {
            ERROR_MESSAGE("Blitzen is already active")
            return;
        }
        else
        {
            pEngineInstance = this;

            if(BlitzenCore::InitLogging())
                DEBUG_MESSAGE("Test Logging")
            else
                ERROR_MESSAGE("Logging is not active")

            INFO_MESSAGE("%s Booting", BLITZEN_VERSION)

            BLIT_ASSERT(BlitzenPlatform::PlatformStartup(&m_platformState, BLITZEN_VERSION, BLITZEN_WINDOW_STARTING_X, 
            BLITZEN_WINDOW_STARTING_Y, m_platformData.windowWidth, m_platformData.windowHeight))

            BLIT_ASSERT(BlitzenCore::InitLogging())

            isRunning = 1;
            isSupended = 0;
        }
    }

    void Engine::Run()
    {
        //Testing allocator, will be removed
        BlitzenCore::BlitAlloc(BlitzenCore::AllocationType::Unkown, 0);
        BlitzenCore::BlitAlloc(BlitzenCore::AllocationType::MaxTypes, 50);

        /* Checking my dynamic array class, will be removed */
        BlitCL::DynamicArray<int> array1;
        BlitCL::DynamicArray<int> array2(10);
        uint32_t size = array1.GetSize();
        uint32_t size1 = array2.GetSize();
        int x = array2[3];
        array2[19] = 10;
        int y = array2[19];
        array1.Resize(20);
        uint32_t size3 = array1.GetSize();
        array2.Resize(3);
        int var = 10;
        array2.PushBack(var);
        int z = array2[10];
        /* Test Dynamic array */

        while(isRunning)
        {
            BlitzenPlatform::PlatformPumpMessages(&m_platformState);
        }
    }

    Engine::~Engine()
    {
        WARNING_MESSAGE("Blitzen is shutting down")
        BlitzenCore::ShutdownLogging();

        BlitzenPlatform::PlatformShutdown(&m_platformState);

        pEngineInstance = nullptr;
    }
}

int main()
{
    BlitzenCore::MemoryManagementInit();

    BlitzenEngine::Engine blitzen;

    BlitzenEngine::Engine shouldNotBeAllowed;

    BlitzenCore::MemoryManagementShutdown();

    blitzen.Run();
}
