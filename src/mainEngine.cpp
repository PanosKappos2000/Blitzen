/* 
    The code in this file calls all of the functions needed to run the applications.
    Contains the main function at the bottom
*/
 
#include "BlitzenVulkan/vulkanRenderer.h"

#include "mainEngine.h"
#include "Platform/platform.h"

inline uint8_t gFreezeFrustum = 0;
inline uint8_t gDebugPyramid = 0;
inline uint8_t gOcclusion = 1;
inline uint8_t gLod = 1;

namespace BlitzenEngine
{
    // This will hold pointer to all the renderers (not really necessary but whatever)
    struct BlitzenRenderers
    {
        BlitzenVulkan::VulkanRenderer* pVulkan = nullptr;
    };

    // Static member variable needs to be declared in the .cpp file as well
    Engine* Engine::pEngineInstance;

    Engine::Engine()
    {
        // There should not be a 2nd instance of Blitzen Engine
        if(GetEngineInstancePointer())
        {
            BLIT_ERROR("Blitzen is already active")
            return;
        }

        // Initialize the engine if it is the first time the constructor is called
        else
        {
            // Initalize the instance and the system boolean to avoid creating or destroying a 2nd instance
            pEngineInstance = this;
            m_systems.engine = 1;
            BLIT_INFO("%s Booting", BLITZEN_VERSION)

            BLIT_ASSERT(BlitzenCore::InitLogging())// This function does nothing at them moment and always returns 1

            if(BlitzenCore::EventsInit())
            {
                BLIT_INFO("Event system active")
                BlitzenCore::InputInit(&m_systems.inputState);// Activate the input system if the event system was succesfully created
            }
            else
                BLIT_FATAL("Event system initialization failed!")

            // This always returns 1 at the moment (I don't know if this would ever fail in the future to be honest)
            BLIT_ASSERT_MESSAGE(BlitzenEngine::LoadResourceSystem(m_resources), "Failed to acquire resourece system")
            

            // Platform specific code initalization. Mostly window(not Windows) related things
            BLIT_ASSERT(BlitzenPlatform::PlatformStartup(BLITZEN_VERSION, BLITZEN_WINDOW_STARTING_X, 
            BLITZEN_WINDOW_STARTING_Y, m_platformData.windowWidth, m_platformData.windowHeight))

            // Register some default events, like window closing on escape and default inputs for camera and debugging
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::EngineShutdown, nullptr, OnEvent);
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::KeyPressed, nullptr, OnKeyPress);
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::KeyReleased, nullptr, OnKeyPress);
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::WindowResize, nullptr, OnResize);
            BlitzenCore::RegisterEvent(BlitzenCore::BlitEventType::MouseMoved, nullptr, OnMouseMove);
        }
    }



    /*!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        This function currently holds the majority of the functionality called during runtime.
        Its scope owns the memory used by each renderer(only Vulkan for the forseeable future.
        It calls every function needed to draw a frame and other functionality the engine has
    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/
    void Engine::Run()
    {
        // Allocate memory for the vulkan renderer, if Vulkan is used (The application does not support anything else anyway)
        #if BLITZEN_VULKAN
            BlitCL::SmartPointer<BlitzenVulkan::VulkanRenderer, BlitzenCore::AllocationType::Renderer> pVulkan;
        #endif

        // The idea with this is that if Blitzen supported more than one graphics API, it would try to initialize every renderer that was requested
        // If no renderer initalization function returned 1, than the engine would fail the assertion
        uint8_t hasRenderer = 0;

        #if BLITZEN_VULKAN
            if(pVulkan.Data())
            {
                // Call the init function and store the result in the systems boolean for Vulkan
                m_systems.vulkan = pVulkan.Data()->Init(m_platformData.windowWidth, m_platformData.windowHeight);
                hasRenderer = m_systems.vulkan;

                // Tell Blitzen to use Vulkan for rendering
                m_renderer = ActiveRenderer::Vulkan;
            }
        #endif

        // Directx12 is here for testing purposes, there's no directx12 backend
        #if BLITZEN_DIRECTX12
            m_systems.directx12 = 1;
            hasRenderer = m_systems.directx12;
            m_renderer = ActiveRenderer::Directx12;
        #endif

        // Test if any renderer was initialized
        BLIT_ASSERT_MESSAGE(hasRenderer, "Blitzen cannot continue without a renderer")
        
        // If the engine passes the above assertion, then it means that it can run the main loop (unless some less fundamental stuff makes it fail)
        isRunning = 1;
        isSupended = 0;

        // Camera and view frustum matrix values
        {
            // Initalize the projection matrix
            m_camera.projectionMatrix = 
            BlitML::InfiniteZPerspective(BLITZEN_FOV, 
            static_cast<float>(m_platformData.windowWidth) / static_cast<float>(m_platformData.windowHeight), 
            BLITZEN_ZNEAR);
            // Give an initial position for the camera and pass it to the view matrix
            BlitML::vec3 initialCameraPosition(50.f, 0.f, 0.f);// Hardcoded, may want to create a macro for this
            m_camera.viewMatrix = BlitML::Mat4Inverse(BlitML::Translate(initialCameraPosition));
            // This is calculated here so that the shaders don't need to calculate this for every invocation
            m_camera.projectionViewMatrix = m_camera.projectionMatrix * m_camera.viewMatrix;

            // The transpose of the projection matrix will be used for frustum culling
            m_camera.projectionTranspose = BlitML::Transpose(m_camera.projectionMatrix);
        }

        /*
            Load the default textures and some other textures for testing
        */
        {
            // Default texture at index 0
            uint32_t blitTexCol = glm::packUnorm4x8(glm::vec4(0.3, 0, 0.6, 1));
            uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	        uint32_t pixels[16 * 16]; 
	        for (int x = 0; x < 16; x++) 
            {
	        	for (int y = 0; y < 16; y++) 
                {
	        		pixels[y*16 + x] = ((x % 2) ^ (y % 2)) ? magenta : blitTexCol;
	        	}
	        }
            m_resources.textures[0].pTextureData = reinterpret_cast<uint8_t*>(pixels);
            m_resources.textures[0].textureHeight = 1;
            m_resources.textures[0].textureWidth = 1;
            m_resources.textures[0].textureChannels = 4;
            m_resources.textureTable.Set(BLIT_DEFAULT_TEXTURE_NAME, &(m_resources.textures[0]));

            // This is hardcoded now
            LoadTextureFromFile(m_resources, "Assets/Textures/cobblestone.png", "loaded_texture", pVulkan.Data(), nullptr);
            LoadTextureFromFile(m_resources, "Assets/Textures/texture.jpg", "loaded_texture2", pVulkan.Data(), nullptr);
            LoadTextureFromFile(m_resources, "Assets/Textures/cobblestone_SPEC.jpg", "spec_texture", pVulkan.Data(), nullptr);
        }

        /*
            Load the default material and some material for testing
        */
        {
            // Manually load a default material at index 0
            m_resources.materials[0].diffuseColor = BlitML::vec4(1.f);
            m_resources.materials[0].diffuseTextureTag = m_resources.textureTable.Get(BLIT_DEFAULT_TEXTURE_NAME, &m_resources.textures[0])->textureTag;
            m_resources.materials[0].specularTextureTag = m_resources.textureTable.Get(BLIT_DEFAULT_TEXTURE_NAME, &m_resources.textures[0])->textureTag;
            m_resources.materials[0].materialId = 0;
            m_resources.materialTable.Set(BLIT_DEFAULT_MATERIAL_NAME, &(m_resources.materials[0]));

            // Test code
            BlitML::vec4 color1(0.1f);
            BlitML::vec4 color2(0.2f);
            DefineMaterial(m_resources, color1, 65.f, "loaded_texture", "spec_texture", "loaded_material");
            DefineMaterial(m_resources, color2, 65.f, "loaded_texture2", "unknown", "loaded_material2");
        }

        // Load test data to draw
        LoadDefaultData(m_resources);

        uint32_t drawCount = BLITZEN_VULKAN_MAX_DRAW_CALLS / 2 + 1;// Rendering a large amount of objects to stress test the renderer

        m_resources.objectCount = drawCount;// Normally the draw count differs from the game object count, but the engine is really simple at the moment
        m_resources.transforms.Resize(m_resources.objectCount);// Every object has a different transform
        // Hardcode a large amount of male model mesh
        for(size_t i = 0; i < m_resources.objectCount / 10; ++i)
        {
            BlitzenEngine::MeshTransform& transform = m_resources.transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1'000 - 50,//x 
            (float(rand()) / RAND_MAX) * 1'000 - 50,//y
            (float(rand()) / RAND_MAX) * 1'000 - 50);//z
            transform.pos = translation;
            transform.scale = 0.1f;

            // Loading random orientation. Normally you would get this from the game object
            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
            (float(rand()) / RAND_MAX) * 2 - 1, // y
            (float(rand()) / RAND_MAX) * 2 - 1); // z
		    float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            transform.orientation = orientation;

            GameObject& currentObject = m_resources.objects[i];

            currentObject.meshIndex = 3;// Hardcode the bunny mesh for each object in this loop
            currentObject.transformIndex = i;// Transform index is the same as the object index
        }
        // Hardcode a large amount of objects with the high polygon kitten mesh and random transforms
        for (size_t i = m_resources.objectCount / 10; i < m_resources.objectCount / 8; ++i)
        {
            BlitzenEngine::MeshTransform& transform = m_resources.transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1'000 - 50,//x 
            (float(rand()) / RAND_MAX) * 1'000 - 50,//y
            (float(rand()) / RAND_MAX) * 1'000 - 50);//z
            transform.pos = translation;
            transform.scale = 0.1f;

            // Loading random orientation. Normally you would get this from the game object
            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
            (float(rand()) / RAND_MAX) * 2 - 1, // y
            (float(rand()) / RAND_MAX) * 2 - 1); // z
		    float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            transform.orientation = orientation;

            GameObject& currentObject = m_resources.objects[i];

            currentObject.meshIndex = 1;// Hardcode the kitten mesh for each object in this loop
            currentObject.transformIndex = i;// Transform index is the same as the object index
        }
        // Hardcode a large amount of stanford dragons
        for (size_t i = m_resources.objectCount / 8; i < m_resources.objectCount / 6; ++i)
        {
            BlitzenEngine::MeshTransform& transform = m_resources.transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1'000 - 50,//x 
                (float(rand()) / RAND_MAX) * 1'000 - 50,//y
                (float(rand()) / RAND_MAX) * 1'000 - 50);//z
            transform.pos = translation;
            transform.scale = 0.1f;

            // Loading random orientation. Normally you would get this from the game object
            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
                (float(rand()) / RAND_MAX) * 2 - 1, // y
                (float(rand()) / RAND_MAX) * 2 - 1); // z
            float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            transform.orientation = orientation;

            GameObject& currentObject = m_resources.objects[i];

            currentObject.meshIndex = 0;// Hardcode the kitten mesh for each object in this loop
            currentObject.transformIndex = i;// Transform index is the same as the object index
        }
        // Hardcode a large amount of standford bunnies
        for (size_t i = m_resources.objectCount / 6; i < m_resources.objectCount; ++i)
        {
            BlitzenEngine::MeshTransform& transform = m_resources.transforms[i];

            // Loading random position and scale. Normally you would get this from the game object
            BlitML::vec3 translation((float(rand()) / RAND_MAX) * 1'000 - 50,//x 
                (float(rand()) / RAND_MAX) * 1'000 - 50,//y
                (float(rand()) / RAND_MAX) * 1'000 - 50);//z
            transform.pos = translation;
            transform.scale = 5.f;

            // Loading random orientation. Normally you would get this from the game object
            BlitML::vec3 axis((float(rand()) / RAND_MAX) * 2 - 1, // x
                (float(rand()) / RAND_MAX) * 2 - 1, // y
                (float(rand()) / RAND_MAX) * 2 - 1); // z
            float angle = BlitML::Radians((float(rand()) / RAND_MAX) * 90.f);
            BlitML::quat orientation = BlitML::QuatFromAngleAxis(axis, angle, 0);
            transform.orientation = orientation;

            GameObject& currentObject = m_resources.objects[i];

            currentObject.meshIndex = 2;// Hardcode the kitten mesh for each object in this loop
            currentObject.transformIndex = i;// Transform index is the same as the object index
        }

        #if BLITZEN_VULKAN
        {
            // The values that were loaded need to be passed to the vulkan renderere so that they can be loaded to GPU buffers
            BlitzenVulkan::GPUData vulkanData(m_resources.vertices, m_resources.indices, m_resources.meshlets, 
            m_resources.surfaces, m_resources.transforms);/* The contructor is needed for values that are references instead of pointers */

            vulkanData.pTextures = m_resources.textures;
            vulkanData.textureCount = m_resources.currentTextureIndex;// Current texture index is equal to the size of the array of textures

            vulkanData.pMaterials = m_resources.materials;
            vulkanData.materialCount = m_resources.currentMaterialIndex;// Current material index is equal to the size of the material array

            vulkanData.pMeshes = m_resources.meshes;
            vulkanData.meshCount = m_resources.currentMeshIndex;// Current mesh index is equal to the size of the mesh array

            vulkanData.pGameObjects = m_resources.objects;
            vulkanData.gameObjectCount = m_resources.objectCount;

            // Draw count will be used to determine the size of draw and object buffers
            vulkanData.drawCount = drawCount;

            pVulkan.Data()->SetupForRendering(vulkanData);
        }
        #endif

        // This is passed to the renderer every frame after updating the data
        // Passed here for debug purposes
        BlitzenVulkan::RenderContext renderContext;

        StartClock();// Start the clock
        double previousTime = m_clock.elapsedTime;// Initialize previous frame time to the elapsed time

        // Main Loop starts
        while(isRunning)
        {
            // Always returns 1 (not sure if I want messages to stop the application ever)
            if(!BlitzenPlatform::PlatformPumpMessages())
            {
                isRunning = 0;
            }

            if(!isSupended)
            {
                // Get the elapsed time of the application
                m_clock.elapsedTime = BlitzenPlatform::PlatformGetAbsoluteTime() - m_clock.startTime;
                // Update the delta time by using the previous elapsed time
                m_deltaTime = m_clock.elapsedTime - previousTime;
                // Update the previous elapsed time to the current elapsed time
                previousTime = m_clock.elapsedTime;

                // With delta time retrieved, call update camera to make any necessary changes to the scene based on its transform
                UpdateCamera(m_camera, (float)m_deltaTime);

                switch(m_renderer)
                {
                    #if BLITZEN_VULKAN
                    case ActiveRenderer::Vulkan:
                    {
                        // In the case that the window was resized, these are passed so that the swapchain can be recreated
                        renderContext.windowResize = m_platformData.windowResize;
                        renderContext.windowWidth = m_platformData.windowWidth;
                        renderContext.windowHeight = m_platformData.windowHeight;

                        // Pass the projection matrix to be used for culling
                        renderContext.projectionMatrix = m_camera.projectionMatrix;
                        // Pass the view matrix to promote bounding spheres to view coordinates
                        // If the freeze frustum global is set to true, this does not get updated, effectively freezing the view frustum
                        // This will not work if the view matrix is used for anything other than view frustum collisions
                        if (!gFreezeFrustum)
                            renderContext.viewMatrix = m_camera.viewMatrix;
                        // The projection view is the precalculated result of projection * view
                        // Calculated on the CPU to avoid doing it every vertex/mesh shader invocation
                        renderContext.projectionView = m_camera.projectionViewMatrix;
                        renderContext.viewPosition = m_camera.position;
                        // The transpose of the projection matrix will be used to derive the frustum planes
                        renderContext.projectionTranspose = m_camera.projectionTranspose;
                        renderContext.zNear = BLITZEN_ZNEAR;

                        // The draw count is passed again every frame even thought is is constant at the moment
                        renderContext.drawCount = drawCount;
                        renderContext.drawDistance = BLITZEN_DRAW_DISTANCE;

                        // Debug values, controlled by inputs
                        renderContext.debugPyramid = gDebugPyramid;// This does nothing now, something is broken with occlusion culling / debug pyramid
                        renderContext.occlusionEnabled = gOcclusion; // f3 to change
                        renderContext.lodEnabled = gLod;// f4 to change

                        // Hardcoding the sun for now (this is used in the fragment shader but ignored)
                        renderContext.sunlightDirection = BlitML::vec3(-0.57735f, -0.57735f, 0.57735f);
                        renderContext.sunlightColor = BlitML::vec4(0.8f, 0.8f, 0.8f, 1.0f);

                        // Let the renderere do its thing
                        pVulkan.Data()->DrawFrame(renderContext);

                        break;
                    }
                    #endif
                    #if BLITZEN_DIRECTX12
                    case ActiveRenderer::Directx12:
                    {
                        BLIT_INFO("Direct 3D not setup, switch to Vulkan")
                        break;
                    }
                    #endif
                    default:
                    {
                        break;
                    }
                }

                // Make sure that the window resize is set to false after the renderer is notified
                m_platformData.windowResize = 0;

                BlitzenCore::UpdateInput(m_deltaTime);
            }
        }
        // Main loop ends
        StopClock();

        // There is not active renderer since the application is shutting down
        m_renderer = ActiveRenderer::MaxRenderers;

        // The renderer is shutdown here because it will go out of scope if this run function goes out of scope
        #if BLITZEN_VULKAN
            m_systems.vulkan = 0;
            pVulkan.Data()->Shutdown();
        #endif

        // With the main loop done, Blitzen calls Shutdown on itself
        Shutdown();
    }

    void Engine::StartClock()
    {
        m_clock.startTime = BlitzenPlatform::PlatformGetAbsoluteTime();
        m_clock.elapsedTime = 0;
    }

    void Engine::StopClock()
    {
        m_clock.elapsedTime = 0;
    }

    void Engine::Shutdown()
    {
        if (m_systems.engine)
        {
            BLIT_WARN("Blitzen is shutting down")

            m_systems.logSystem = 0;
            BlitzenCore::ShutdownLogging();

            m_systems.eventSystem = 0;
            BlitzenCore::EventsShutdown();

            m_systems.inputSystem = 0;
            BlitzenCore::InputShutdown();

            BlitzenPlatform::PlatformShutdown();

            m_systems.engine = 0;
            pEngineInstance = nullptr;
        }

        // The destructor should not be calle more than once as it will crush the application
        else
        {
            BLIT_ERROR("Any uninitialized instances of Blitzen will not be explicitly cleaned up")
        }
    }




    // This will move from here once I add a camera system
    void UpdateCamera(Camera& camera, float deltaTime)
    {
        if (camera.cameraDirty)
        {
            // I haven't overloaded the += operator
            BlitML::vec3 finalVelocity = camera.velocity * deltaTime * 40.f;
            BlitML::vec4 directionalVelocity = camera.rotation * BlitML::vec4(finalVelocity);
            camera.position = camera.position + BlitML::ToVec3(directionalVelocity);
            camera.translation = BlitML::Translate(camera.position);

            camera.viewMatrix = BlitML::Mat4Inverse(camera.translation * camera.rotation);
            camera.projectionViewMatrix = camera.projectionMatrix * camera.viewMatrix;
        }
    }

    void RotateCamera(Camera& camera, float deltaTime, float pitchRotation, float yawRotation)
    {
        // find how much the camera's yaw moved
        if(yawRotation < 100.f && yawRotation > -100.f)
        {
            camera.yawRotation += yawRotation * 10.f * deltaTime / 100.f;
        }
        // Find how much the camera's pitch moved
        if(pitchRotation < 100.f && pitchRotation > -100.f)
        {
            camera.pitchRotation -= pitchRotation * 10.f * deltaTime / 100.f;
        }

        // Find the yaw orientation of the camera using quaternions
        BlitML::vec3 yAxis(0.f, -1.f, 0.f);
        BlitML::quat yawOrientation = BlitML::QuatFromAngleAxis(yAxis, camera.yawRotation, 0);

        // Find the pitch orientation of the camera using quaternions
        BlitML::vec3 xAxis(1.f, 0.f, 0.f);
        BlitML::quat pitchOrientation = BlitML::QuatFromAngleAxis(xAxis, camera.pitchRotation, 0);

        // Turn the quaternions to matrices and multiply them to get the new rotation matrix
        BlitML::mat4 yawRot = BlitML::QuatToMat4(yawOrientation);
        BlitML::mat4 pitchRot = BlitML::QuatToMat4(pitchOrientation);
        camera.rotation = yawRot * pitchRot;
    }

    uint8_t OnEvent(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data)
    {
        if(eventType == BlitzenCore::BlitEventType::EngineShutdown)
        {
            BLIT_WARN("Engine shutdown event encountered!")
            BlitzenEngine::Engine::GetEngineInstancePointer()->RequestShutdown();
            return 1; 
        }

        return 0;
    }

    uint8_t OnKeyPress(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data)
    {
        //Get the key pressed from the event context
        BlitzenCore::BlitKey key = static_cast<BlitzenCore::BlitKey>(data.data.ui16[0]);

        if(eventType == BlitzenCore::BlitEventType::KeyPressed)
        {
            switch(key)
            {
                case BlitzenCore::BlitKey::__ESCAPE:
                {
                    BlitzenCore::EventContext newContext = {};
                    BlitzenCore::FireEvent(BlitzenCore::BlitEventType::EngineShutdown, nullptr, newContext);
                    return 1;
                }
                // The four keys below control basic camera movement. They set a velocity and tell it that it should be updated based on that velocity
                case BlitzenCore::BlitKey::__W:
                {
                    Camera& camera = Engine::GetEngineInstancePointer()->GetCamera();
                    camera.cameraDirty = 1;
                    camera.velocity = BlitML::vec3(0.f, 0.f, 1.f);
                    break;
                }
                case BlitzenCore::BlitKey::__S:
                {
                    Camera& camera = Engine::GetEngineInstancePointer()->GetCamera();
                    camera.cameraDirty = 1;
                    camera.velocity = BlitML::vec3(0.f, 0.f, -1.f);
                    break;
                }
                case BlitzenCore::BlitKey::__A:
                {
                    Camera& camera = Engine::GetEngineInstancePointer()->GetCamera();
                    camera.cameraDirty = 1;
                    camera.velocity = BlitML::vec3(-1.f, 0.f, 0.f);
                    break;
                }
                case BlitzenCore::BlitKey::__D:
                {
                    Camera& camera = Engine::GetEngineInstancePointer()->GetCamera();
                    camera.cameraDirty = 1;
                    camera.velocity = BlitML::vec3(1.f, 0.f, 0.f);
                    break;
                }
                case BlitzenCore::BlitKey::__F1:
                {
                    gFreezeFrustum = !gFreezeFrustum;
                    break;
                }
                case BlitzenCore::BlitKey::__F2:
                {
                    gDebugPyramid = !gDebugPyramid;
                    break;
                }
                case BlitzenCore::BlitKey::__F3:
                {
                    gOcclusion = !gOcclusion;
                    break;
                }
                case BlitzenCore::BlitKey::__F4:
                {
                    gLod = !gLod;
                    break;
                }
                default:
                {
                    BLIT_DBLOG("Key pressed %i", key)
                    return 1;
                }
            }
        }
        else if (eventType == BlitzenCore::BlitEventType::KeyReleased)
        {
            switch (key)
            {
                case BlitzenCore::BlitKey::__W:
                case BlitzenCore::BlitKey::__S:
                {
                    Camera& camera = Engine::GetEngineInstancePointer()->GetCamera();
                    camera.velocity.z = 0.f;
                    if(camera.velocity.y == 0.f && camera.velocity.x == 0.f)
                    {
                        camera.cameraDirty = 0;
                    }
                    break;
                }
                case BlitzenCore::BlitKey::__A:
                case BlitzenCore::BlitKey::__D:
                {
                    Camera& camera = Engine::GetEngineInstancePointer()->GetCamera();
                    camera.velocity.x = 0.f;
                    if (camera.velocity.y == 0.f && camera.velocity.z == 0.f)
                    {
                        camera.cameraDirty = 0;
                    }
                    break;
                }
            }
        }
        return 0;
    }

    uint8_t OnResize(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data)
    {
        uint32_t newWidth = data.data.ui32[0];
        uint32_t newHeight = data.data.ui32[1];

        Engine::GetEngineInstancePointer()->UpdateWindowSize(newWidth, newHeight);

        return 1;
    }

    uint8_t OnMouseMove(BlitzenCore::BlitEventType eventType, void* pSender, void* pListener, BlitzenCore::EventContext data)
    {
        Camera& camera = Engine::GetEngineInstancePointer()->GetCamera();
        float deltaTime = static_cast<float>(Engine::GetEngineInstancePointer()->GetDeltaTime());

        camera.cameraDirty = 1;

        RotateCamera(camera, deltaTime, data.data.si16[1], data.data.si16[0]);

        return 1;
    }

    void Engine::UpdateWindowSize(uint32_t newWidth, uint32_t newHeight) 
    {
        m_platformData.windowWidth = newWidth; 
        m_platformData.windowHeight = newHeight;
        m_platformData.windowResize = 1;
        if(newWidth == 0 || newHeight == 0)
        {
            isSupended = 1;
            return;
        }
        isSupended = 0;

        m_camera.projectionMatrix = BlitML::InfiniteZPerspective(BlitML::Radians(70.f), static_cast<float>(m_platformData.windowWidth) / 
        static_cast<float>(m_platformData.windowHeight), BLITZEN_ZNEAR);
        m_camera.projectionViewMatrix = m_camera.projectionMatrix * m_camera.viewMatrix;
        m_camera.projectionTranspose = BlitML::Transpose(m_camera.projectionMatrix);
    }
}







int main()
{
    BlitzenCore::MemoryManagementInit();

    // Blitzen engine Allocated and freed inside this scope
    {
        // Blitzen engine allocated on the heap, it will cause stack overflow otherwise
        BlitCL::SmartPointer<BlitzenEngine::Engine, BlitzenCore::AllocationType::Engine> Blitzen;

        Blitzen.Data()->Run();
    }

    BlitzenCore::MemoryManagementShutdown();
}
