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
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
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
            //If this is a debug build, the validation layer extension is also needed
            #ifndef NDEBUG

                requiredExtensionNames[2] = "VK_EXT_debug_utils";

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
            instanceInfo.enabledLayerCount = 0;
             
            instanceInfo.pApplicationInfo = &applicationInfo;

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
                        m_graphicsQueue.index = i;
                        m_graphicsQueue.hasIndex = 1;
                    }

                    // Checks for a compute queue index, if one has not already been found 
                    if(queueFamilyProperties[i].queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT && !m_computeQueue.hasIndex)
                    {
                        m_computeQueue.index = i;
                        m_computeQueue.hasIndex = 1;
                    }

                    VkBool32 supportsPresent = VK_FALSE;
                    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(pdv, i, m_initHandles.surface, &supportsPresent))
                    if(supportsPresent == VK_TRUE && !m_presentQueue.hasIndex)
                    {
                        m_presentQueue.index = i;
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

                //Retrieve queue families from device
                VkPhysicalDeviceProperties2 props{};
                props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                vkGetPhysicalDeviceProperties2(pdv, &props);

                // Prefer discrete gpu if there is one
                if(props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
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
            deviceInfo.pEnabledFeatures = &deviceFeatures;

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

            void* pNextChain [3] = {&vulkan11Features, &vulkan12Features, &vulkan13Features};
            deviceInfo.pNext = *pNextChain;

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
        {

            // This will be needed to find some details about swapchain support
            VkPhysicalDeviceSurfaceInfo2KHR surfaceInfo{};
            surfaceInfo.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SURFACE_INFO_2_KHR;
            surfaceInfo.surface = m_initHandles.surface;

            VkSwapchainCreateInfoKHR swapchainInfo{};
            swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainInfo.pNext = nullptr;
            swapchainInfo.flags = 0;
            swapchainInfo.imageArrayLayers = 1;
            swapchainInfo.clipped = VK_TRUE;// Don't present things renderer out of bounds
            swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            swapchainInfo.surface = m_initHandles.surface;
            swapchainInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            swapchainInfo.oldSwapchain = VK_NULL_HANDLE;

            /* Image format and color space */
            {
                // Retrieve surface format to check for desired swapchain format and color space
                uint32_t surfaceFormatsCount = 0; 
                VK_CHECK(vkGetPhysicalDeviceSurfaceFormats2KHR(m_initHandles.chosenGpu, &surfaceInfo, &surfaceFormatsCount, nullptr))
                BlitCL::DynamicArray<VkSurfaceFormat2KHR> surfaceFormats(static_cast<size_t>(surfaceFormatsCount));
                VK_CHECK(vkGetPhysicalDeviceSurfaceFormats2KHR(m_initHandles.chosenGpu, &surfaceInfo, &surfaceFormatsCount, surfaceFormats.Data()))
                // Look for the desired image format
                uint8_t found = 0;
                for(size_t i = 0; i < surfaceFormats.GetSize(); ++i)
                {
                    // If the desire image format is found assign it to the swapchain info and break out of the loop
                    if(surfaceFormats[i].surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && 
                    surfaceFormats[i].surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                    {
                        swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
                        swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                        // Save the format to init handles
                        m_initHandles.swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
                        found = 1;
                        break;
                    }
                }

                // If the desired format is not found (unlikely), assign the first one that is supported and hope for the best ( I'm just a chill guy )
                if(!found)
                {
                    swapchainInfo.imageFormat = surfaceFormats[0].surfaceFormat.format;
                    // Save the image format
                    m_initHandles.swapchainFormat = swapchainInfo.imageFormat;
                    swapchainInfo.imageColorSpace = surfaceFormats[0].surfaceFormat.colorSpace;
                }
            }

            /* Present mode */
            {
                // Retrieve the presentation modes supported, to look for the desired one
                uint32_t presentModeCount = 0;
                VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_initHandles.chosenGpu, m_initHandles.surface, &presentModeCount, nullptr));
                BlitCL::DynamicArray<VkPresentModeKHR> presentModes(static_cast<size_t>(presentModeCount));
                VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(m_initHandles.chosenGpu, m_initHandles.surface, &presentModeCount, presentModes.Data()))
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
            m_initHandles.swapchainExtent = {windowWidth, windowHeight};
            // Retrieve surface capabilities to properly configure some swapchain values
            VkSurfaceCapabilities2KHR surfaceCapabilities{};
            surfaceCapabilities.sType = VK_STRUCTURE_TYPE_SURFACE_CAPABILITIES_2_KHR;
            VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilities2KHR(m_initHandles.chosenGpu, &surfaceInfo, &surfaceCapabilities));

            /* Swapchain extent */
            {
                // Get the swapchain extent from the surface capabilities, if it is not some weird value
                if(surfaceCapabilities.surfaceCapabilities.currentExtent.width != UINT32_MAX)
                {
                    m_initHandles.swapchainExtent = surfaceCapabilities.surfaceCapabilities.currentExtent;
                }

                // Get the min extent and max extent allowed by the GPU,  to clamp the initial value
                VkExtent2D minExtent = surfaceCapabilities.surfaceCapabilities.minImageExtent;
                VkExtent2D maxExtent = surfaceCapabilities.surfaceCapabilities.maxImageExtent;
                m_initHandles.swapchainExtent.width = BlitCL::Clamp(m_initHandles.swapchainExtent.width, maxExtent.width, minExtent.width);
                m_initHandles.swapchainExtent.height = BlitCL::Clamp(m_initHandles.swapchainExtent.height, maxExtent.width, minExtent.height);

                // Swapchain extent fully checked and ready to pass to the swapchain info
                swapchainInfo.imageExtent = m_initHandles.swapchainExtent;
            }

            /* Min image count */
            {
                uint32_t imageCount = surfaceCapabilities.surfaceCapabilities.minImageCount + 1;
                // Check that image count did not supass max image count
                if(surfaceCapabilities.surfaceCapabilities.maxImageCount > 0 && surfaceCapabilities.surfaceCapabilities.maxImageCount < imageCount )
                {
                    imageCount = surfaceCapabilities.surfaceCapabilities.maxImageCount;
                }

                // Swapchain image count fully check and ready to be pass to the swapchain info
                swapchainInfo.minImageCount = imageCount;
            }

            swapchainInfo.preTransform = surfaceCapabilities.surfaceCapabilities.currentTransform;

            if (m_graphicsQueue.index != m_presentQueue.index)
            {
                uint32_t queueFamilyIndices[] = {m_graphicsQueue.index, m_presentQueue.index};
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
            VK_CHECK(vkCreateSwapchainKHR(m_device, &swapchainInfo, m_pCustomAllocator, &m_initHandles.swapchain));

            // Retrieve the swapchain images
            uint32_t swapchainImageCount = 0;
            vkGetSwapchainImagesKHR(m_device, m_initHandles.swapchain, &swapchainImageCount, nullptr);
            m_initHandles.swapchainImages.Resize(swapchainImageCount);
            vkGetSwapchainImagesKHR(m_device, m_initHandles.swapchain, &swapchainImageCount, m_initHandles.swapchainImages.Data());

            // Create image view for each swapchain image
            m_initHandles.swapchainImageViews.Resize(static_cast<size_t>(swapchainImageCount));
            for(size_t i = 0; i < m_initHandles.swapchainImageViews.GetSize(); ++i)
            {
                VkImageViewCreateInfo info{};
                info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                info.pNext = nullptr;
                info.flags = 0;
                info.image = m_initHandles.swapchainImages[i];
                info.format = m_initHandles.swapchainFormat;
                info.viewType = VK_IMAGE_VIEW_TYPE_2D;
                info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; // Not sure if this is needed, as a seperate color attachment will be used 
                info.subresourceRange.baseMipLevel = 1;
                info.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
                info.subresourceRange.baseArrayLayer = 1;
                info.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

                VK_CHECK(vkCreateImageView(m_device, &info, m_pCustomAllocator, &(m_initHandles.swapchainImageViews[i])))
            }
        }
    }


    void VulkanRenderer::Shutdown()
    {
        for(size_t i = 0; i < m_initHandles.swapchainImageViews.GetSize(); ++i)
        {
            vkDestroyImageView(m_device, m_initHandles.swapchainImageViews[i], m_pCustomAllocator);
        }
        vkDestroySwapchainKHR(m_device, m_initHandles.swapchain, m_pCustomAllocator);
        vkDestroyDevice(m_device, m_pCustomAllocator);
        vkDestroySurfaceKHR(m_initHandles.instance, m_initHandles.surface, m_pCustomAllocator);
        DestroyDebugUtilsMessengerEXT(m_initHandles.instance, m_initHandles.debugMessenger, m_pCustomAllocator);
        vkDestroyInstance(m_initHandles.instance, m_pCustomAllocator);
    }
}