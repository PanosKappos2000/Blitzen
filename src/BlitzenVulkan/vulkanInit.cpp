#include "vulkanRenderer.h"
#include "Platform/platform.h"

namespace BlitzenVulkan
{
    /*
        These function are used load the function pointer for creating the debug messenger
    */
    VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) 
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        } else 
        {
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }
    void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) 
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) 
        {
            func(instance, debugMessenger, pAllocator);
        }
    }
    // Debug messenger callback function
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
    VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) 
    {
        BLIT_ERROR("Validation layer: %s", pCallbackData->pMessage) 
        return VK_FALSE;
    }
    // Validation layers function pointers




    void VulkanRenderer::Init(void* pPlatformState, uint32_t windowWidth, uint32_t windowHeight)
    {
        /*
            If Blitzen ever uses a custom allocator it should be initalized here
        */
        m_pCustomAllocator = nullptr;



        /*------------------------
            VkInstance Creation
        -------------------------*/
        {
            //Will be passed to the VkInstanceCreateInfo that will create Vulkan's instance
            VkApplicationInfo applicationInfo{};
            applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            applicationInfo.pNext = nullptr; // Not using this
            applicationInfo.apiVersion = VK_API_VERSION_1_3; // There are some features and extensions Blitzen will use, that exist in Vulkan 1.3
            // These are not important right now but passing them either way, to be thorough
            applicationInfo.pApplicationName = BLITZEN_VULKAN_USER_APPLICATION;
            applicationInfo.applicationVersion = BLITZEN_VULKAN_USER_APPLICATION_VERSION;
            applicationInfo.pEngineName = BLITZEN_VULKAN_USER_ENGINE;
            applicationInfo.engineVersion = BLITZEN_VULKAN_USER_ENGINE_VERSION;

            VkInstanceCreateInfo instanceInfo {};
            instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            instanceInfo.pNext = nullptr; // Will be used if validation layers are activated later in this function
            instanceInfo.flags = 0; // Not using this 
            instanceInfo.pApplicationInfo = &applicationInfo;

            // TODO: Use what is stored in the dynamic arrays below to check if all extensions are supported
            uint32_t extensionsCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
            BlitCL::DynamicArray<VkExtensionProperties> availableExtensions(static_cast<size_t>(extensionsCount));
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableExtensions.Data());

            // Creating an array of required extension names to pass to ppEnabledExtensionNames
            const char* requiredExtensionNames [BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT];
            requiredExtensionNames[0] =  VULKAN_SURFACE_KHR_EXTENSION_NAME;
            requiredExtensionNames[1] = "VK_KHR_surface";        
            instanceInfo.enabledExtensionCount = BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT;
            instanceInfo.enabledLayerCount = 0; // Validation layers inactive at first, but will be activated if it's a debug build

            //If this is a debug build, the validation layer extension is also needed
            #ifndef NDEBUG

                requiredExtensionNames[2] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

                // Getting all supported validation layers
                uint32_t availableLayerCount = 0;
                vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
                BlitCL::DynamicArray<VkLayerProperties> availableLayers(static_cast<size_t>(availableLayerCount));
                vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.Data());

                // Checking if the requested validation layers are supported
                uint8_t layersFound = 0;
                for(size_t i = 0; i < availableLayers.GetSize(); i++)
                {
                   if(strcmp(availableLayers[i].layerName,VALIDATION_LAYER_NAME))
                   {
                       layersFound = 1;
                       break;
                   }
                }

                BLIT_ASSERT_MESSAGE(layersFound, "The vulkan renderer will not be used in debug mode without validation layers")

                // If the above check is passed validation layers can be safely loaded
                instanceInfo.enabledLayerCount = 1;
                const char* layerNameRef = VALIDATION_LAYER_NAME;
                instanceInfo.ppEnabledLayerNames = &layerNameRef;

                VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
                debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                debugMessengerInfo.pfnUserCallback = debugCallback;
                debugMessengerInfo.pNext = nullptr;// Not using this right now
                debugMessengerInfo.pUserData = nullptr; // Not using this right now

                // The debug messenger needs to be referenced by the instance
                instanceInfo.pNext = &debugMessengerInfo;

            #endif

            instanceInfo.ppEnabledExtensionNames = requiredExtensionNames;

            VK_CHECK(vkCreateInstance(&instanceInfo, m_pCustomAllocator, &(m_initHandles.instance)));
        }
        /*--------------------------------------------------
            VkInstance created and stored in m_initHandles
        ----------------------------------------------------*/



        #ifndef NDEBUG
        /*---------------------------------
            Debug messenger creation 
        ----------------------------------*/
        {
            // This is the same as the one loaded to VkInstanceCreateInfo
            VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
            debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debugMessengerInfo.pfnUserCallback = debugCallback;
            debugMessengerInfo.pNext = nullptr;// Not using this right now
            debugMessengerInfo.pUserData = nullptr; // Not using this right now

            VK_CHECK(CreateDebugUtilsMessengerEXT(m_initHandles.instance, &debugMessengerInfo, m_pCustomAllocator, &(m_initHandles.debugMessenger)));
        }
        /*--------------------------------------------------------
            Debug messenger created and stored in m_initHandles
        ----------------------------------------------------------*/
        #endif


        {
            BlitzenPlatform::PlatformState* pTrueState = reinterpret_cast<BlitzenPlatform::PlatformState*>(pPlatformState);
            BlitzenPlatform::CreateVulkanSurface(pTrueState, m_initHandles.instance, m_initHandles.surface, m_pCustomAllocator);
        }




        
        /*
            Physical device (GPU representation) selection
        */
        {
            uint32_t physicalDeviceCount = 0;
            vkEnumeratePhysicalDevices(m_initHandles.instance, &physicalDeviceCount, nullptr);
            BLIT_ASSERT(physicalDeviceCount)
            BlitCL::DynamicArray<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
            vkEnumeratePhysicalDevices(m_initHandles.instance, &physicalDeviceCount, physicalDevices.Data());

            // Goes through the available devices, to eliminate the ones that are completely inadequate
            for(size_t i = 0; i < physicalDevices.GetSize(); ++i)
            {
                VkPhysicalDevice& pdv = physicalDevices[i];

                //Retrieve queue families from device
                uint32_t queueFamilyPropertyCount = 0;
                vkGetPhysicalDeviceQueueFamilyProperties2(pdv, &queueFamilyPropertyCount, nullptr);
                // Remove this device from the candidates, if no queue families were retrieved
                if(!queueFamilyPropertyCount)
                {
                    physicalDevices.RemoveAtIndex(i);
                    --i;
                    continue;
                }
                // Store the queue family properties to query for their indices
                BlitCL::DynamicArray<VkQueueFamilyProperties2> queueFamilyProperties(static_cast<size_t>(queueFamilyPropertyCount));
                for(size_t i = 0; i < queueFamilyProperties.GetSize(); ++i)
                {
                    queueFamilyProperties[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
                }
                vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevices[i], &queueFamilyPropertyCount, queueFamilyProperties.Data());
                for(size_t i = 0; i < queueFamilyProperties.GetSize(); ++i)
                {
                    // Checks for a graphics queue index, if one has not already been found 
                    if(queueFamilyProperties[i].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT && !m_graphicsQueue.hasIndex)
                    {
                        m_graphicsQueue.index = static_cast<uint32_t>(i);
                        m_graphicsQueue.hasIndex = 1;
                    }

                    // Checks for a compute queue index, if one has not already been found 
                    if(queueFamilyProperties[i].queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT && !m_computeQueue.hasIndex)
                    {
                        m_computeQueue.index = static_cast<uint32_t>(i);
                        m_computeQueue.hasIndex = 1;
                    }

                    VkBool32 supportsPresent = VK_FALSE;
                    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(pdv, static_cast<uint32_t>(i), m_initHandles.surface, &supportsPresent))
                    if(supportsPresent == VK_TRUE && !m_presentQueue.hasIndex)
                    {
                        m_presentQueue.index = static_cast<uint32_t>(i);
                        m_presentQueue.hasIndex = 1;
                    }
                }

                // If one of the required queue families has no index, then it gets removed from the candidates
                if(!m_presentQueue.hasIndex || !m_graphicsQueue.hasIndex || !m_computeQueue.hasIndex)
                {
                    physicalDevices.RemoveAtIndex(i);
                    --i;
                }

                // TODO: Query for feature support
            }

            for(size_t i = 0; i < physicalDevices.GetSize(); ++i)
            {
                VkPhysicalDevice& pdv = physicalDevices[i];

                // Retrieve properties from device
                VkPhysicalDeviceProperties props{};
                vkGetPhysicalDeviceProperties(pdv, &props);

                // Prefer discrete gpu if there is one
                if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                {
                    m_initHandles.chosenGpu = pdv;
                    m_stats.hasDiscreteGPU = 1;
                }
            }

            // If a discrete GPU is not found, the renderer chooses the 1st device. This will change the way the renderer goes forward
            if(!m_stats.hasDiscreteGPU)
                m_initHandles.chosenGpu = physicalDevices[0];

        }
        /*
            Chosen physical device saved to m_chosenGpu
        */



        /*-----------------------
            Device creation
        -------------------------*/
        {
            VkDeviceCreateInfo deviceInfo{};
            deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            deviceInfo.flags = 0; // Not using this
            deviceInfo.enabledLayerCount = 0;//Deprecated

            // Only using the swapchain extension for now
            deviceInfo.enabledExtensionCount = 1;
            const char* extensionsNames = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
            deviceInfo.ppEnabledExtensionNames = &extensionsNames;

            // Standard device features
            VkPhysicalDeviceFeatures deviceFeatures{};
            #if BLITZEN_VULKAN_INDIRECT_DRAW
                deviceFeatures.multiDrawIndirect = true;
            #endif
            deviceInfo.pEnabledFeatures = nullptr;

            // Extended device features
            VkPhysicalDeviceVulkan11Features vulkan11Features{};
            vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            #if BLITZEN_VULKAN_INDIRECT_DRAW
                vulkan11Features.shaderDrawParameters = true;
            #endif

            VkPhysicalDeviceVulkan12Features vulkan12Features{};
            vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            vulkan12Features.bufferDeviceAddress = true;
            vulkan12Features.descriptorIndexing = true;
            //Allows vulkan to reset queries
            vulkan12Features.hostQueryReset = true;
            //Allows spir-v shaders to use descriptor arrays
            vulkan12Features.runtimeDescriptorArray = true;

            VkPhysicalDeviceVulkan13Features vulkan13Features{};
            vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            //Using dynamic rendering since the engine will not benefit from the VkRenderPass
            vulkan13Features.dynamicRendering = true;
            vulkan13Features.synchronization2 = true;

            VkPhysicalDeviceFeatures2 vulkanExtendedFeatures{};
            vulkanExtendedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            vulkanExtendedFeatures.features = deviceFeatures;

            deviceInfo.pNext = &vulkanExtendedFeatures;
            vulkanExtendedFeatures.pNext = &vulkan11Features;
            vulkan11Features.pNext = &vulkan12Features;
            vulkan12Features.pNext = &vulkan13Features;

            BlitCL::DynamicArray<VkDeviceQueueCreateInfo> queueInfos(1);
            queueInfos[0].queueFamilyIndex = m_graphicsQueue.index;
            // If compute has a different index from present, add a new info for it
            if(m_graphicsQueue.index != m_computeQueue.index)
            {
                queueInfos.PushBack(VkDeviceQueueCreateInfo());
                queueInfos[1].queueFamilyIndex = m_computeQueue.index;
            }
            // If an info was created for compute and present is not equal to compute or graphics, create a new one for present as well
            if(queueInfos.GetSize() == 2 && queueInfos[0].queueFamilyIndex != m_presentQueue.index && queueInfos[1].queueFamilyIndex != m_presentQueue.index)
            {
                queueInfos.PushBack(VkDeviceQueueCreateInfo());
                queueInfos[2].queueFamilyIndex = m_presentQueue.index;
            }
            // If an info was not created for compute but present has a different index from the other 2, create a new info for it
            if(queueInfos.GetSize() == 1 && queueInfos[0].queueFamilyIndex != m_presentQueue.index)
            {
                queueInfos.PushBack(VkDeviceQueueCreateInfo());
                queueInfos[1].queueFamilyIndex = m_presentQueue.index;
            }
            // With the count of the queue infos found and the indices passed, the rest is standard
            float priority = 1.f;
            for(size_t i = 0; i < queueInfos.GetSize(); ++i)
            {
                queueInfos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueInfos[i].pNext = nullptr; // Not using this
                queueInfos[i].flags = 0; // Not using this
                queueInfos[i].queueCount = 1;
                queueInfos[i].pQueuePriorities = &priority;
            }
            // Pass the queue infos
            deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueInfos.GetSize());
            deviceInfo.pQueueCreateInfos = queueInfos.Data();

            // Create the device
            VK_CHECK(vkCreateDevice(m_initHandles.chosenGpu, &deviceInfo, m_pCustomAllocator, &m_device));

            // Retrieve graphics queue handle
            VkDeviceQueueInfo2 graphicsQueueInfo{};
            graphicsQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
            graphicsQueueInfo.pNext = nullptr; // Not using this
            graphicsQueueInfo.flags = 0; // Not using this
            graphicsQueueInfo.queueFamilyIndex = m_graphicsQueue.index;
            graphicsQueueInfo.queueIndex = 0;
            vkGetDeviceQueue2(m_device, &graphicsQueueInfo, &m_graphicsQueue.handle);

            // Retrieve compute queue handle
            VkDeviceQueueInfo2 computeQueueInfo{};
            computeQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
            computeQueueInfo.pNext = nullptr; // Not using this
            computeQueueInfo.flags = 0; // Not using this
            computeQueueInfo.queueFamilyIndex = m_computeQueue.index;
            computeQueueInfo.queueIndex = 0;
            vkGetDeviceQueue2(m_device, &computeQueueInfo, &m_computeQueue.handle);

            // Retrieve present queue handle
            VkDeviceQueueInfo2 presentQueueInfo{};
            presentQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
            presentQueueInfo.pNext = nullptr; // Not using this
            presentQueueInfo.flags = 0; // Not using this
            presentQueueInfo.queueFamilyIndex = m_presentQueue.index;
            presentQueueInfo.queueIndex = 0;
            vkGetDeviceQueue2(m_device, &presentQueueInfo, &m_presentQueue.handle);
        }
        /*
            Device created and saved to m_device. Queue handles retrieved
        */


        /*
             Swapchain creation
        */
       CreateSwapchain(m_device, m_initHandles, windowWidth, windowHeight, m_graphicsQueue, m_presentQueue, m_computeQueue, m_pCustomAllocator);


        /* Create the vma allocator for vulkan resource allocation */
        {
            VmaAllocatorCreateInfo allocatorInfo{};
            allocatorInfo.device = m_device;
            allocatorInfo.instance = m_initHandles.instance;
            allocatorInfo.physicalDevice = m_initHandles.chosenGpu;
            allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
            VK_CHECK(vmaCreateAllocator(&allocatorInfo, &m_allocator));
        }



        /* Allocating temporary command buffer, since some commands will be needed outside of the draw loop*/
        {
            VkCommandPoolCreateInfo commandPoolCreateInfo{};
            commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            // This will allow each command buffer created by this pool to be inidividually reset
            commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            commandPoolCreateInfo.pNext = nullptr;
            commandPoolCreateInfo.queueFamilyIndex = m_graphicsQueue.index;
            VK_CHECK(vkCreateCommandPool(m_device, &commandPoolCreateInfo, nullptr, &m_placeholderCommandPool));

            VkCommandBufferAllocateInfo commandBufferInfo{};
            commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferInfo.pNext = nullptr;
            commandBufferInfo.commandPool = m_placeholderCommandPool;
            commandBufferInfo.commandBufferCount = 1;
            commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            VK_CHECK(vkAllocateCommandBuffers(m_device, &commandBufferInfo, &m_placeholderCommands))
        }

        // This will be referred to by rendering attachments and will be updated when the window is resized
        m_drawExtent = {windowWidth, windowHeight};
    }

    void CreateSwapchain(VkDevice device, InitializationHandles& initHandles, uint32_t windowWidth, uint32_t windowHeight, 
    Queue graphicsQueue, Queue presentQueue, Queue computeQueue, VkAllocationCallbacks* pCustomAllocator)
    {
            VkSwapchainCreateInfoKHR swapchainInfo{};
            swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainInfo.pNext = nullptr;
            swapchainInfo.flags = 0;
            swapchainInfo.imageArrayLayers = 1;
            swapchainInfo.clipped = VK_TRUE;// Don't present things renderer out of bounds
            swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchainInfo.surface = initHandles.surface;
            swapchainInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

            /* Image format and color space */
            {
                // Retrieve surface format to check for desired swapchain format and color space
                uint32_t surfaceFormatsCount = 0; 
                VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(initHandles.chosenGpu, initHandles.surface, &surfaceFormatsCount, nullptr))
                BlitCL::DynamicArray<VkSurfaceFormatKHR> surfaceFormats(static_cast<size_t>(surfaceFormatsCount));
                VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(initHandles.chosenGpu, initHandles.surface, &surfaceFormatsCount, surfaceFormats.Data()))
                // Look for the desired image format
                uint8_t found = 0;
                for(size_t i = 0; i < surfaceFormats.GetSize(); ++i)
                {
                    // If the desire image format is found assign it to the swapchain info and break out of the loop
                    if(surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM && 
                    surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                    {
                        swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
                        swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                        // Save the format to init handles
                        initHandles.swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
                        found = 1;
                        break;
                    }
                }

                // If the desired format is not found (unlikely), assign the first one that is supported and hope for the best ( I'm just a chill guy )
                if(!found)
                {
                    swapchainInfo.imageFormat = surfaceFormats[0].format;
                    // Save the image format
                    initHandles.swapchainFormat = swapchainInfo.imageFormat;
                    swapchainInfo.imageColorSpace = surfaceFormats[0].colorSpace;
                }
            }

            /* Present mode */
            {
                // Retrieve the presentation modes supported, to look for the desired one
                uint32_t presentModeCount = 0;
                VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(initHandles.chosenGpu, initHandles.surface, &presentModeCount, nullptr));
                BlitCL::DynamicArray<VkPresentModeKHR> presentModes(static_cast<size_t>(presentModeCount));
                VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(initHandles.chosenGpu, initHandles.surface, &presentModeCount, presentModes.Data()))
                // Look for the desired presentation mode
                uint8_t found = 0;
                for(size_t i = 0; i < presentModes.GetSize(); ++i)
                {
                    // If the desired presentation mode is found, set the swapchain info to that
                    if(presentModes[i] == DESIRED_SWAPCHAIN_PRESENTATION_MODE)
                    {
                        swapchainInfo.presentMode = DESIRED_SWAPCHAIN_PRESENTATION_MODE;
                        found = 1;
                        break;
                    }
                }
                
                // If it was not found, set it to this random smuck ( Don't worry, I'm a proffesional )
                if(!found)
                {
                    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
                }
            }

            //Set the swapchain extent to the window's width and height
            initHandles.swapchainExtent = {windowWidth, windowHeight};
            // Retrieve surface capabilities to properly configure some swapchain values
            VkSurfaceCapabilitiesKHR surfaceCapabilities{};
            VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(initHandles.chosenGpu, initHandles.surface, &surfaceCapabilities));

            /* Swapchain extent */
            {
                // Get the swapchain extent from the surface capabilities, if it is not some weird value
                if(surfaceCapabilities.currentExtent.width != UINT32_MAX)
                {
                    initHandles.swapchainExtent = surfaceCapabilities.currentExtent;
                }

                // Get the min extent and max extent allowed by the GPU,  to clamp the initial value
                VkExtent2D minExtent = surfaceCapabilities.minImageExtent;
                VkExtent2D maxExtent = surfaceCapabilities.maxImageExtent;
                initHandles.swapchainExtent.width = BlitCL::Clamp(initHandles.swapchainExtent.width, maxExtent.width, minExtent.width);
                initHandles.swapchainExtent.height = BlitCL::Clamp(initHandles.swapchainExtent.height, maxExtent.height, minExtent.height);

                // Swapchain extent fully checked and ready to pass to the swapchain info
                swapchainInfo.imageExtent = initHandles.swapchainExtent;
            }

            /* Min image count */
            {
                uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
                // Check that image count did not supass max image count
                if(surfaceCapabilities.maxImageCount > 0 && surfaceCapabilities.maxImageCount < imageCount )
                {
                    imageCount = surfaceCapabilities.maxImageCount;
                }

                // Swapchain image count fully check and ready to be pass to the swapchain info
                swapchainInfo.minImageCount = imageCount;
            }

            swapchainInfo.preTransform = surfaceCapabilities.currentTransform;

            if (graphicsQueue.index != presentQueue.index)
            {
                uint32_t queueFamilyIndices[] = {graphicsQueue.index, presentQueue.index};
                swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                swapchainInfo.queueFamilyIndexCount = 2;
                swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
            } 
            else 
            {
                swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                swapchainInfo.queueFamilyIndexCount = 0;// Unnecessary if the indices are the same
            }

            // Create the swapchain
            VK_CHECK(vkCreateSwapchainKHR(device, &swapchainInfo, pCustomAllocator, &initHandles.swapchain));

            // Retrieve the swapchain images
            uint32_t swapchainImageCount = 0;
            vkGetSwapchainImagesKHR(device, initHandles.swapchain, &swapchainImageCount, nullptr);
            initHandles.swapchainImages.Resize(swapchainImageCount);
            vkGetSwapchainImagesKHR(device, initHandles.swapchain, &swapchainImageCount, initHandles.swapchainImages.Data());

            // Create image view for each swapchain image
            initHandles.swapchainImageViews.Resize(static_cast<size_t>(swapchainImageCount));
            for(size_t i = 0; i < initHandles.swapchainImageViews.GetSize(); ++i)
            {
                VkImageViewCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.pNext = nullptr;
                info.flags = 0;
                info.image = initHandles.swapchainImages[i];
                info.format = initHandles.swapchainFormat;
                info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Not sure if this is needed, as a seperate color attachment will be used 
                info.subresourceRange.baseMipLevel = 0;
                info.subresourceRange.levelCount = 1;
                info.subresourceRange.baseArrayLayer = 0;
                info.subresourceRange.layerCount = 1;

                VK_CHECK(vkCreateImageView(device, &info, pCustomAllocator, &(initHandles.swapchainImageViews[i])))
            }
    }


    void VulkanRenderer::Shutdown()
    {
        vkDeviceWaitIdle(m_device);

        for(size_t i = 0; i < m_loadedTextures.GetSize(); ++i)
        {
            m_loadedTextures[i].image.CleanupResources(m_allocator, m_device);

            vkDestroySampler(m_device, m_loadedTextures[i].sampler, m_pCustomAllocator);
        }

        vkDestroyDescriptorPool(m_device, m_textureDescriptorAllocator, m_pCustomAllocator);
        vkDestroyDescriptorSetLayout(m_device, m_textureDescriptorSetLayout, m_pCustomAllocator);

        vmaDestroyBuffer(m_allocator, m_staticRenderObjectBuffer.buffer, m_staticRenderObjectBuffer.allocation);
        vmaDestroyBuffer(m_allocator, m_globalVertexBuffer.buffer, m_globalVertexBuffer.allocation);
        vmaDestroyBuffer(m_allocator, m_globalIndexBuffer.buffer, m_globalIndexBuffer.allocation);

        vkDestroyDescriptorSetLayout(m_device, m_globalShaderDataLayout, m_pCustomAllocator);

        vkDestroyPipeline(m_device, m_opaqueGraphicsPipeline, m_pCustomAllocator);
        vkDestroyPipelineLayout(m_device, m_opaqueGraphicsPipelineLayout, m_pCustomAllocator);

        m_depthAttachment.CleanupResources(m_allocator, m_device);
        m_colorAttachment.CleanupResources(m_allocator, m_device);

        for(size_t i = 0; i < BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT; ++i)
        {
            FrameTools& frameTools = m_frameToolsList[i];
            vkDestroyCommandPool(m_device, frameTools.mainCommandPool, m_pCustomAllocator);

            vkDestroyFence(m_device, frameTools.inFlightFence, m_pCustomAllocator);
            vkDestroySemaphore(m_device, frameTools.imageAcquiredSemaphore, m_pCustomAllocator);
            vkDestroySemaphore(m_device, frameTools.readyToPresentSemaphore, m_pCustomAllocator);

            vkDestroyDescriptorPool(m_device, frameTools.globalShaderDataDescriptorPool, m_pCustomAllocator);
            vmaDestroyBuffer(m_allocator, frameTools.globalShaderDataBuffer.buffer, frameTools.globalShaderDataBuffer.allocation);
        }

        vkDestroyCommandPool(m_device, m_placeholderCommandPool, m_pCustomAllocator);

        for(size_t i = 0; i < m_initHandles.swapchainImageViews.GetSize(); ++i)
        {
            vkDestroyImageView(m_device, m_initHandles.swapchainImageViews[i], m_pCustomAllocator);
        }
        vkDestroySwapchainKHR(m_device, m_initHandles.swapchain, m_pCustomAllocator);

        vmaDestroyAllocator(m_allocator);

        vkDestroyDevice(m_device, m_pCustomAllocator);
        vkDestroySurfaceKHR(m_initHandles.instance, m_initHandles.surface, m_pCustomAllocator);
        DestroyDebugUtilsMessengerEXT(m_initHandles.instance, m_initHandles.debugMessenger, m_pCustomAllocator);
        vkDestroyInstance(m_initHandles.instance, m_pCustomAllocator);
    }
}