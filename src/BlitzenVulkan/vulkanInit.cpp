#include "vulkanRenderer.h"
#include "Platform/platform.h"
#include <cstring> // For strcmp

namespace BlitzenVulkan
{
    // Platform specific expressions
    #if defined(_WIN32)
        constexpr const char* ce_surfaceExtensionName  = "VK_KHR_win32_surface";
        constexpr const char* ce_baseValidationLayerName =  "VK_LAYER_KHRONOS_validation";                 
    #elif linux
        constexpr const char* ce_surfaceExtensionName = "VK_KHR_xcb_surface";      
        constexpr const char* ce_baseValidationLayerName = "VK_LAYER_NV_optimus";                  
        #define VK_USE_PLATFORM_XCB_KHR
    #endif

    VulkanRenderer* VulkanRenderer::m_pThisRenderer = nullptr;

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
        switch (messageSeverity)
        {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        {
            BLIT_INFO("Validation layer: %s", pCallbackData->pMessage);
            return VK_FALSE;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        {
            BLIT_WARN("Validation layer: %s", pCallbackData->pMessage)
                return VK_FALSE;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        {
            BLIT_ERROR("Validation layer: %s", pCallbackData->pMessage)
                return VK_FALSE;
        }
        default:
            return VK_FALSE;
        }
    }

    VulkanRenderer::VulkanRenderer() :
        m_pCustomAllocator{ nullptr }, m_debugMessenger {VK_NULL_HANDLE}, 
        m_currentFrame{ 0 }, m_loadingTriangleVertexColor{ 0.1f, 0.8f, 0.3f }, 
        m_depthPyramidMipLevels{ 0 }, textureCount{ 0 }
    {}

    uint8_t VulkanRenderer::Init(uint32_t windowWidth, uint32_t windowHeight)
    {
        m_pCustomAllocator = nullptr;

        // Save the renderer's instance 
        m_pThisRenderer = this;

        // Creates the Vulkan instance
        if(!CreateInstance(m_instance, &m_debugMessenger))
        {
            BLIT_ERROR("Failed to create vulkan instance")
            return 0;
        }

        // Create the surface depending on the implementation on Platform.cpp
        if(!BlitzenPlatform::CreateVulkanSurface(m_instance, m_surface.handle, m_pCustomAllocator))
        {
            BLIT_ERROR("Failed to create Vulkan window surface")
            return 0;
        }

        // Call the function to search for a suitable physical device, it it can't find one return 0
        if(!PickPhysicalDevice(m_physicalDevice, m_instance, m_surface.handle, 
        m_graphicsQueue, m_computeQueue, m_presentQueue, m_transferQueue, m_stats))
        {
            BLIT_ERROR("Failed to pick suitable physical device")
            return 0;
        }

        // Create the device
        if(!CreateDevice(m_device, m_physicalDevice, m_graphicsQueue, 
            m_presentQueue, m_computeQueue, m_transferQueue, m_stats
        ))
        {
            BLIT_ERROR("Failed to pick suitable physical device")
        }

        // Creates the swapchain
        if(!CreateSwapchain(m_device, m_surface.handle, m_physicalDevice, 
        windowWidth, windowHeight, m_graphicsQueue, m_presentQueue, m_computeQueue, 
        m_pCustomAllocator, m_swapchainValues))
        {
            BLIT_ERROR("Failed to create Vulkan swapchain")
            return 0;
        }

        // This will be referred to by rendering attachments and will be updated when the window is resized
        m_drawExtent = {m_swapchainValues.swapchainExtent.width, m_swapchainValues.swapchainExtent.height};

        
		if (!CreateIdleDrawHandles())
		{
			BLIT_ERROR("Failed to create idle draw handles")
		    return 0;
		}
        
        return 1;
    }

    uint8_t CreateInstance(VkInstance& instance, VkDebugUtilsMessengerEXT* pDM /*=nullptr*/)
    {
        // Check if the driver supports vulkan 1.3. The engine requires for Vulkan to be in 1.3
        uint32_t apiVersion = 0;
        VK_CHECK(vkEnumerateInstanceVersion(&apiVersion));
        if(apiVersion < VK_API_VERSION_1_3)
        {
            BLIT_ERROR("Blitzen needs to use Vulkan API_VERSION 1.3")
            return 0;
        }

        //Will be passed to the VkInstanceCreateInfo that will create Vulkan's instance
        VkApplicationInfo applicationInfo{};
        applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        applicationInfo.pNext = nullptr; 
        applicationInfo.apiVersion = VK_API_VERSION_1_3; 
        applicationInfo.pApplicationName = ce_userApp;
        applicationInfo.applicationVersion = ce_appVersion;
        applicationInfo.pEngineName = ce_hostEngine;
        applicationInfo.engineVersion = ce_userEngineVersion;

        VkInstanceCreateInfo instanceInfo {};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pNext = nullptr; // Will be used if validation layers are activated later in this function
        instanceInfo.flags = 0; // Not using this 
        instanceInfo.pApplicationInfo = &applicationInfo;

        // Checking that all required instance extensions are supported
        uint32_t availableExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
        BlitCL::DynamicArray<VkExtensionProperties> availableExtensions(static_cast<size_t>(availableExtensionCount));
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.Data());

        // Store the names of all possible extensions to be used
        const char* possibleRequestedExtensions[ce_maxRequestedInstanceExtensions] = {
            ce_surfaceExtensionName, "VK_KHR_surface", VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        };
        // Stores a boolean for each extension, telling the application if they were requested, if they are required and if they were found
        uint8_t extensionsSupportRequested[ce_maxRequestedInstanceExtensions] = 
        {
            1, 
            1, 
            ce_bValidationLayersRequested
        };
        uint8_t extensionSupportRequired[ce_maxRequestedInstanceExtensions] = {1, 1, 0};
        uint8_t extensionSupportFound[ce_maxRequestedInstanceExtensions] = { 0, 0, 0 };

        // Final extension count
        const char* extensionNames[ce_maxRequestedInstanceExtensions];
        uint32_t extensionCount = 0;

        for(size_t i = 0; i < ce_maxRequestedInstanceExtensions; ++i)
        {
            if(!extensionsSupportRequested[i])
                continue;
                
            for(auto& extension : availableExtensions)
            {
                // Check for surafce extension support
                if(!strcmp(extension.extensionName, possibleRequestedExtensions[i]))
                {
                    extensionNames[extensionCount++] = extension.extensionName;
                    extensionSupportFound[i] = 1;
                }
            }
            if(!extensionSupportFound[i] && extensionSupportRequired[i])
            {
                BLIT_ERROR("Vulkan instance exetension with name: %s was not found", possibleRequestedExtensions[i])
                return 0;
            }
            // TODO: I could throw a warning for non required extension that were not found

        }

        instanceInfo.ppEnabledExtensionNames = extensionNames;        
        instanceInfo.enabledExtensionCount = extensionCount;
        instanceInfo.enabledLayerCount = 0; // Validation layers inactive at first, but will be activated if it's a debug build

        // Keeps the debug messenger info and the validation layers enabled boolean here, they will be needed later
        uint8_t validationLayersEnabled = 0;
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
        // If validation layers are requested, the debut utils extension is also needed
        if (ce_bValidationLayersRequested)
        {
            if (extensionSupportFound[ce_maxRequestedInstanceExtensions - 1])
            {
                const char* layerNameRef[2] = { ce_baseValidationLayerName, "VK_LAYER_KHRONOS_synchronization2" };
                if (EnableInstanceValidation(debugMessengerInfo))
                {
                    // The debug messenger needs to be referenced by the instance
                    instanceInfo.pNext = &debugMessengerInfo;

                    // If the layer for synchronization 2 is found, it enables that as well
                    if (ce_bSynchronizationValidationRequested && EnabledInstanceSynchronizationValidation())
                        instanceInfo.enabledLayerCount = 2;
                    // If not just enables the other one
                    else
                        instanceInfo.enabledLayerCount = 1;

                    instanceInfo.ppEnabledLayerNames = layerNameRef;
                    validationLayersEnabled = 1;
                }
            }
        }

        VkResult res = vkCreateInstance(&instanceInfo, nullptr, &instance);
        if(res != VK_SUCCESS)
            return 0;

        // If validation layers are request and they were succesfully found in the VkInstance earlier, the debug messenger is created
        if(validationLayersEnabled)
            CreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, pDM);

        return 1;
    }

    uint8_t EnableInstanceValidation(VkDebugUtilsMessengerCreateInfoEXT& debugMessengerInfo)
    {
        // Getting all supported validation layers
        uint32_t availableLayerCount = 0;
        vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
        BlitCL::DynamicArray<VkLayerProperties> availableLayers(static_cast<size_t>(availableLayerCount));
        vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.Data());

        // Checking if the requested validation layers are supported
        uint8_t layersFound = 0;
        for(size_t i = 0; i < availableLayers.GetSize(); i++)
        {
           if(!strcmp(availableLayers[i].layerName,ce_baseValidationLayerName))
           {
               layersFound = 1;
               break;
           }
        }

        if(!layersFound)
        {
            BLIT_ERROR("The vulkan renderer should not be used in debug mode without validation layers")
            return 0;
        }
        
        // Create the debug messenger
        debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

        debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

        debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        // Debug messenger callback function defined at the top of this file
        debugMessengerInfo.pfnUserCallback = debugCallback;

        debugMessengerInfo.pNext = nullptr;// Not using this right now
        debugMessengerInfo.pUserData = nullptr; // Not using this right now

        return 1;
    }

    uint8_t EnabledInstanceSynchronizationValidation()
    {
        // Getting all supported validation layers
        uint32_t availableLayerCount = 0;
        vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
        BlitCL::DynamicArray<VkLayerProperties> availableLayers(static_cast<size_t>(availableLayerCount));
        vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.Data());

        // Checking if the requested validation layers are supported
        uint8_t layersFound = 0;
        for (size_t i = 0; i < availableLayers.GetSize(); i++)
        {
            if (!strcmp(availableLayers[i].layerName, "VK_LAYER_KHRONOS_synchronization2"))
            {
                layersFound = 1;
                break;
            }
        }

        return layersFound;
    }

    uint8_t PickPhysicalDevice(VkPhysicalDevice& gpu, VkInstance instance, VkSurfaceKHR surface,
    Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, Queue& transferQueue, 
    VulkanStats& stats)
    {
        // Retrieves the physical device count
        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
        if(!physicalDeviceCount)
            return 0;

        // Pass the available devices to an array to pick the best one
        BlitCL::DynamicArray<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.Data());

        uint32_t extensionCount = 0;

        // Goes through the available devices, to eliminate the ones that are completely inadequate
        for(auto& pdv : physicalDevices)
        {
            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(pdv, &props);
            // Only choose discrete GPUs at first
            if (props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
                continue;
            // Checks if the discrete gpu is suitable
            if(ValidatePhysicalDevice(pdv, instance, surface, graphicsQueue, 
                computeQueue, presentQueue, transferQueue, stats
            ))
            {
                gpu = pdv;
                stats.hasDiscreteGPU = 1;
                BLIT_INFO("Discrete GPU found");
                return 1;
            }

            // Reset the extension count, in case it was touched by the previous device before it was rejected
            stats.deviceExtensionCount = 0;
        }

        BLIT_INFO("Discrete GPU not found, looking for fallback");
        for(auto& pdv : physicalDevices)
        {
            // Checks for possible non discrete GPUs
            if(!ValidatePhysicalDevice(pdv, instance, surface, graphicsQueue, 
                computeQueue, presentQueue, transferQueue, stats))
            {
                gpu = pdv;
                return 1;
            }

            // Reset the extension count, in case it was touched by the previous device before it was rejected
            stats.deviceExtensionCount = 0;
        }

        // If the function has reached this point, it means that it has failed
        return 0;
    }

    uint8_t ValidatePhysicalDevice(VkPhysicalDevice pdv, VkInstance instance, VkSurfaceKHR surface, 
    Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, Queue& transferQueue, 
    VulkanStats& stats)
    {
        // Get core physical device features
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(pdv, &features);

        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(pdv, &props);
        if (props.apiVersion < VK_API_VERSION_1_3)
            return 0;

        // Get newer version physical Device Features
        VkPhysicalDeviceFeatures2 features2{};
        VkPhysicalDeviceVulkan11Features features11{};
        features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        // Add Vulkan 1.1 features to the pNext chain
        features2.pNext = &features11;
        VkPhysicalDeviceVulkan12Features features12{};
        features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        // Add Vulkan 1.2 features to the pNext chain
        features11.pNext = &features12;
        VkPhysicalDeviceVulkan13Features features13{};
        features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        // Add Vulkan 1.3 features to the pNext chain
        features12.pNext = &features13;
        VkPhysicalDeviceMeshShaderFeaturesNV featuresMesh{};
        featuresMesh.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
        // Add Nvidia mesh shader features to the pNext chain
        features13.pNext = &featuresMesh;
        vkGetPhysicalDeviceFeatures2(pdv, &features2);

        // Check that all the required features are supported by the device
        if(!features.multiDrawIndirect || !features.samplerAnisotropy ||
            // Vulkan 1.1 features
            !features11.storageBuffer16BitAccess || !features11.shaderDrawParameters ||
            // Vulkan 1.2 features
            !features12.bufferDeviceAddress || 
            !features12.descriptorIndexing || 
            !features12.runtimeDescriptorArray ||  
            !features12.storageBuffer8BitAccess || 
            !features12.shaderFloat16 || 
            !features12.drawIndirectCount ||
            !features12.samplerFilterMinmax || 
            !features12.shaderInt8 || 
            !features12.shaderSampledImageArrayNonUniformIndexing ||
            !features12.uniformAndStorageBuffer8BitAccess || 
            !features12.storagePushConstant8 ||
            // Vulkan 1.3 features
            !features13.synchronization2 || !features13.dynamicRendering || !features13.maintenance4
        )
            return 0;
        
        // Looks for the requested extensions. Fails if the required ones are not found
        if(!LookForRequestedExtensions(pdv, stats))
            return 0;

        //Retrieve queue families from device
        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(pdv, &queueFamilyPropertyCount, nullptr);
        // Remove this device from the candidates, if no queue families were retrieved
        if(!queueFamilyPropertyCount)
            return 0;

        // Store the queue family properties to query for their indices
        BlitCL::DynamicArray<VkQueueFamilyProperties2> queueFamilyProperties(
        static_cast<size_t>(queueFamilyPropertyCount), std::move(VkQueueFamilyProperties2({})));

        for(auto& queueProps : queueFamilyProperties)
        {
            queueProps.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        }

        vkGetPhysicalDeviceQueueFamilyProperties2(pdv, &queueFamilyPropertyCount, queueFamilyProperties.Data());

        uint32_t queueIndex = 0;
        for (auto& queueProps : queueFamilyProperties)
        {
            // Checks for a graphics queue index, if one has not already been found 
            if (queueProps.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphicsQueue.index = queueIndex;
                graphicsQueue.hasIndex = 1;
                break;
            }

            ++queueIndex;
        }

        queueIndex = 0;
        for (auto& queueProps : queueFamilyProperties)
        {
            // Checks for a compute queue index, if one has not already been found 
            if (queueProps.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT)
            {
                computeQueue.index = queueIndex;
                computeQueue.hasIndex = 1;
                break;
            }

            ++queueIndex;
        }

		queueIndex = 0;
        for (auto& queueProps : queueFamilyProperties)
        {
            // Checks for a transfer queue index, if one has not already been found
            if (queueProps.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT && 
                queueIndex != graphicsQueue.index
            )
            {
                transferQueue.index = queueIndex;
                transferQueue.hasIndex = 1;
                break;
            }

            ++queueIndex;
        }

		queueIndex = 0;
		for (auto& queueProps : queueFamilyProperties)
        {
            // Checks for presentation queue, if one was not already found
            VkBool32 supportsPresent = VK_FALSE;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(pdv, queueIndex, surface, &supportsPresent))
            if(supportsPresent == VK_TRUE && !presentQueue.hasIndex)
            {
                presentQueue.index = queueIndex;
                presentQueue.hasIndex = 1;
                break;
            }

            ++queueIndex;
        }

        // If one of the required queue families has no index, then it gets removed from the candidates
        if(!presentQueue.hasIndex || !graphicsQueue.hasIndex || !computeQueue.hasIndex || !transferQueue.hasIndex)
            return 0;

        return 1;
        
    }

    uint8_t LookForRequestedExtensions(VkPhysicalDevice pdv, VulkanStats& stats)
    {
        // Checking if the device supports all extensions that will be requested from Vulkan
        uint32_t dvExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(pdv, nullptr, &dvExtensionCount, nullptr);
        BlitCL::DynamicArray<VkExtensionProperties> dvExtensionsProps(static_cast<size_t>(dvExtensionCount));
        vkEnumerateDeviceExtensionProperties(pdv, nullptr, &dvExtensionCount, dvExtensionsProps.Data());

        struct ExtensionQueryHelper
        {
            const char* extensionName;
            uint8_t bSupportRequested;
            uint8_t bSupportRequired;
            uint8_t bSupportFound;

            inline ExtensionQueryHelper(const char* name, uint8_t bRequested, uint8_t bRequired)
                :extensionName{name}, bSupportRequested{bRequested}, 
                bSupportRequired{bRequired}, bSupportFound{0}
            {}

        };
        ExtensionQueryHelper extensionsData [ce_maxRequestedDeviceExtensions]
        {
            { VK_KHR_SWAPCHAIN_EXTENSION_NAME, /*requested*/1, /*required*/1 },
            { VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, /*requested*/1, /*required*/1 },
            { VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, /*requested*/ce_bRaytracing, /*required*/0 }, 
            { VK_KHR_RAY_QUERY_EXTENSION_NAME, /*requested*/ce_bRaytracing, /*required*/0 }, 
            { VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, /*requested*/ce_bRaytracing, /*required*/0 }, 
            { VK_EXT_MESH_SHADER_EXTENSION_NAME, /*requested*/ce_bMeshShaders, /*required*/0 }
        };

        // Check for the required extension name with strcmp
        for(auto& data : extensionsData)
        {
            // If the extensions was never requested, it does not bother checking for it
            if(!data.bSupportRequested)
                continue;

            for(auto& extension : dvExtensionsProps)
            {
                if(!strcmp(extension.extensionName, data.extensionName))
                {
                    data.bSupportFound = 1;
                    stats.deviceExtensionNames[stats.deviceExtensionCount++] = data.extensionName;
                }  
            }

            if(!data.bSupportFound && data.bSupportRequired)
            {
                BLIT_ERROR("Device extension with name: %s, not supported", data.extensionName)
                return 0;
            }
        }

        // Check for mesh shaders features and extensions
        if (ce_bMeshShaders)
        {

            // Check for mesh shader feature in available features
            VkPhysicalDeviceFeatures2 features2{};
            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            VkPhysicalDeviceMeshShaderFeaturesEXT meshFeatures{};
            meshFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
            features2.pNext = &meshFeatures;
            vkGetPhysicalDeviceFeatures2(pdv, &features2);
            
            // Mesh shaders are supported if both the feature and the extensions are found
            stats.meshShaderSupport = extensionsData[5].extensionName 
            && meshFeatures.meshShader && meshFeatures.taskShader;

            if (stats.meshShaderSupport)
            {
                BLIT_INFO("Mesh shader support confirmed");
            }
            else
            {
                BLIT_INFO("No mesh shader support, using traditional pipeline");
                #undef BLIT_VK_MESH_EXT
            }
        }

        // Checks for raytracing extensions and features
        if(ce_bRaytracing)
        {
            VkPhysicalDeviceFeatures2 features2{};
            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            VkPhysicalDeviceRayQueryFeaturesKHR rayQuery{};
            rayQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
            features2.pNext = &rayQuery;
            VkPhysicalDeviceAccelerationStructureFeaturesKHR ASfeats{};
            ASfeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            rayQuery.pNext = &ASfeats;
            vkGetPhysicalDeviceFeatures2(pdv, &features2);

            stats.bRayTracingSupported = 
            extensionsData[2].extensionName && 
            extensionsData[3].extensionName && 
            extensionsData[4].extensionName &&
            rayQuery.rayQuery && ASfeats.accelerationStructure;

            if (stats.bRayTracingSupported)
            {
                BLIT_INFO("Ray tracing support confirmed");
            }
            else
            {
                BLIT_INFO("No ray tracing support found, using traditional raster");
                #undef BLIT_VK_RAYTRACING
            }
        }

        return 1;
    }

    uint8_t CreateDevice(VkDevice& device, VkPhysicalDevice physicalDevice, Queue& graphicsQueue, 
    Queue& presentQueue, Queue& computeQueue, Queue& transferQueue, VulkanStats& stats)
    {
        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.flags = 0; // Not using this
        deviceInfo.enabledLayerCount = 0;//Deprecated

        // Vulkan should ignore the mesh shader extension if support for it was not found
        deviceInfo.enabledExtensionCount = stats.deviceExtensionCount;
        deviceInfo.ppEnabledExtensionNames = stats.deviceExtensionNames;

        // Standard device features
        VkPhysicalDeviceFeatures deviceFeatures{};

        // Allows the renderer to use one vkCmdDrawIndrect type call for multiple objects
        deviceFeatures.multiDrawIndirect = true;

        // Allows sampler anisotropy to be VK_TRUE when creating a VkSampler
        deviceFeatures.samplerAnisotropy = true;

        // Extended device features
        VkPhysicalDeviceVulkan11Features vulkan11Features{};
        vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

        vulkan11Features.shaderDrawParameters = true;

        // Allows use of 16 bit types inside storage buffers in the shaders
        vulkan11Features.storageBuffer16BitAccess = true;

        VkPhysicalDeviceVulkan12Features vulkan12Features{};
        vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

        // Allow the application to get the address of a buffer and pass it to the shaders
        vulkan12Features.bufferDeviceAddress = true;

        // Allows the shaders to index into array held by descriptors, needed for textures
        vulkan12Features.descriptorIndexing = true;

        // Allows shaders to use array with undefined size for descriptors, needed for textures
        vulkan12Features.runtimeDescriptorArray = true;

        // Allows the use of float16_t type in the shaders
        vulkan12Features.shaderFloat16 = true;

        // Allows the use of 8 bit integers in shaders
        vulkan12Features.shaderInt8 = true;

        // Allows storage buffers to have 8bit data
        vulkan12Features.storageBuffer8BitAccess = true;

        // Allows push constants to have 8bit data
        vulkan12Features.storagePushConstant8 = true;

        // Allows the use of draw indirect count, which has the power to completely removes unneeded draw calls
        vulkan12Features.drawIndirectCount = true;

        // This is needed to create a sampler for the depth pyramid that will be used for occlusion culling
        vulkan12Features.samplerFilterMinmax = true;

        // Allows indexing into non uniform sampler arrays
        vulkan12Features.shaderSampledImageArrayNonUniformIndexing = true;

        // Allows uniform buffers to have 8bit members
        vulkan12Features.uniformAndStorageBuffer8BitAccess = true;

        VkPhysicalDeviceVulkan13Features vulkan13Features{};
        vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        // Dynamic rendering removes the need for VkRenderPass and allows the creation of rendering attachmets at draw time
        vulkan13Features.dynamicRendering = true;

        // Used for PipelineBarrier2, better sync structure API
        vulkan13Features.synchronization2 = true;

        // This is needed for local size id in shaders
        vulkan13Features.maintenance4 = true;

        VkPhysicalDeviceMeshShaderFeaturesEXT vulkanFeaturesMesh{};
        vulkanFeaturesMesh.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
        vulkanFeaturesMesh.meshShader = false;
        vulkanFeaturesMesh.taskShader = false;
        #if defined(BLIT_VK_MESH_EXT)
            vulkanFeaturesMesh.meshShader = true;
            vulkanFeaturesMesh.taskShader = true;
        #endif

        VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
        rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};
        accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        rayQueryFeatures.rayQuery = false;
        accelerationStructureFeatures.accelerationStructure = false;
        if(stats.bRayTracingSupported)
        {
            rayQueryFeatures.rayQuery = true;
            accelerationStructureFeatures.accelerationStructure = true;
        }

        VkPhysicalDeviceFeatures2 vulkanExtendedFeatures{};
        vulkanExtendedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        vulkanExtendedFeatures.features = deviceFeatures;

        // Adds all features structs to the pNext chain
        deviceInfo.pNext = &vulkanExtendedFeatures;
        vulkanExtendedFeatures.pNext = &vulkan11Features;
        vulkan11Features.pNext = &vulkan12Features;
        vulkan12Features.pNext = &vulkan13Features;
        vulkan13Features.pNext = &vulkanFeaturesMesh;
        vulkanFeaturesMesh.pNext = &rayQueryFeatures;
        rayQueryFeatures.pNext = &accelerationStructureFeatures;

        BlitCL::DynamicArray<VkDeviceQueueCreateInfo> queueInfos(1);
        queueInfos[0].queueFamilyIndex = graphicsQueue.index;
        VkDeviceQueueCreateInfo deviceQueueInfo{}; // temp
        queueInfos.PushBack(deviceQueueInfo);
        queueInfos[1].queueFamilyIndex = transferQueue.index;
        
        // If compute has a different index from present, add a new info for it
        if(graphicsQueue.index != presentQueue.index)
        {
            queueInfos.PushBack(deviceQueueInfo);
            queueInfos[2].queueFamilyIndex = presentQueue.index;
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
        VkResult res = vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device);
        if(res != VK_SUCCESS)
            return 0;

        // Retrieve graphics queue handle
        VkDeviceQueueInfo2 graphicsQueueInfo{};
        graphicsQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
        graphicsQueueInfo.pNext = nullptr; // Not using this
        graphicsQueueInfo.flags = 0; // Not using this
        graphicsQueueInfo.queueFamilyIndex = graphicsQueue.index;
        graphicsQueueInfo.queueIndex = 0;
        vkGetDeviceQueue2(device, &graphicsQueueInfo, &graphicsQueue.handle);

        // Retrieve compute queue handle
        VkDeviceQueueInfo2 computeQueueInfo{};
        computeQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
        computeQueueInfo.pNext = nullptr; // Not using this
        computeQueueInfo.flags = 0; // Not using this
        computeQueueInfo.queueFamilyIndex = computeQueue.index;
        computeQueueInfo.queueIndex = 0;
        vkGetDeviceQueue2(device, &computeQueueInfo, &computeQueue.handle);

        // Retrieve present queue handle
        VkDeviceQueueInfo2 presentQueueInfo{};
        presentQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
        presentQueueInfo.pNext = nullptr; // Not using this
        presentQueueInfo.flags = 0; // Not using this
        presentQueueInfo.queueFamilyIndex = presentQueue.index;
        presentQueueInfo.queueIndex = 0;
        vkGetDeviceQueue2(device, &presentQueueInfo, &presentQueue.handle);

        VkDeviceQueueInfo2 transferQueueInfo{};
		transferQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
		transferQueueInfo.flags = 0; // Not using this
		transferQueueInfo.pNext = nullptr; // Not using this
        transferQueueInfo.queueFamilyIndex = transferQueue.index;
        transferQueueInfo.queueIndex = 0;
		vkGetDeviceQueue2(device, &transferQueueInfo, &transferQueue.handle);

        return 1;
    }

    uint8_t CreateSwapchain(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice,
    uint32_t windowWidth, uint32_t windowHeight, Queue graphicsQueue, Queue presentQueue, Queue computeQueue, 
    VkAllocationCallbacks* pCustomAllocator, Swapchain& newSwapchain, VkSwapchainKHR oldSwapchain /*=VK_NULL_HANDLE*/)
    {
        VkSwapchainCreateInfoKHR swapchainInfo{};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.pNext = nullptr;
        swapchainInfo.flags = 0;
        swapchainInfo.imageArrayLayers = 1;
        // Don't present things that are out of bounds
        swapchainInfo.clipped = VK_TRUE;
        // Might not be supported in some cases, so I might want to guard against this
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.surface = surface;
        // The color attachment will transfer its contents to the swapchain image when rendering is done
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT 
            | VK_IMAGE_USAGE_STORAGE_BIT 
            | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        // Used when the swapchain is recreated
        swapchainInfo.oldSwapchain = oldSwapchain;

        // Finds the surface format, updates the swapchain info and swapchain struct if it succeeds
        if(!FindSwapchainSurfaceFormat(physicalDevice, surface, swapchainInfo, newSwapchain.swapchainFormat))
            return 0;

        // Finds the present mode, updates the swapchain info if it succeeds
        if(!FindSwapchainPresentMode(physicalDevice, surface, swapchainInfo))
            return 0;

        // Sets the swapchain extent to the window's width and height
        newSwapchain.swapchainExtent = {windowWidth, windowHeight};

        // Compare the current swapchain stats to the surface capabilities
        if(!FindSwapchainSurfaceCapabilities(physicalDevice, surface, swapchainInfo, newSwapchain))
            return 0;

        uint32_t queueFamilyIndices[] = { graphicsQueue.index, presentQueue.index };
        // Configure queue settings based on if the graphics queue also supports presentation
        if (graphicsQueue.index != presentQueue.index)
        {
            swapchainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainInfo.queueFamilyIndexCount = 2;
            swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
        } 
        else 
        {
            swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainInfo.queueFamilyIndexCount = 0;// Unnecessary if the indices are the same
            swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;// Some devices fail if I do not do this.
            // But with queueFamilyIndexCount being 0, this should be fine
        }

        // Create the swapchain
        VkResult swapchainResult = vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, 
        &(newSwapchain.swapchainHandle));
        if(swapchainResult != VK_SUCCESS)
            return 0;

        // Retrieve the swapchain image count
        uint32_t swapchainImageCount = 0;
        VkResult imageResult = vkGetSwapchainImagesKHR(device, 
        newSwapchain.swapchainHandle, &swapchainImageCount, nullptr);
        if(imageResult != VK_SUCCESS)
            return 0;

        // Resize the swapchain images array and pass the swapchain images
        newSwapchain.swapchainImages.Resize(swapchainImageCount);
        imageResult = vkGetSwapchainImagesKHR(device, 
        newSwapchain.swapchainHandle, &swapchainImageCount, newSwapchain.swapchainImages.Data());
        if(imageResult != VK_SUCCESS)
            return 0;

        // Creates image views for the swapchain
        if(newSwapchain.swapchainImageViews.GetSize() == 0)
        {
            newSwapchain.swapchainImageViews.Resize(newSwapchain.swapchainImages.GetSize());
            newSwapchain.swapchainImageViews.Fill(std::move(VK_NULL_HANDLE));
        }
        for(size_t i = 0; i < newSwapchain.swapchainImages.GetSize(); ++i)
        {
            auto& view = newSwapchain.swapchainImageViews[i];
            if(view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device, view, nullptr);
            }
            if(!CreateImageView(device, view, newSwapchain.swapchainImages[i], 
                newSwapchain.swapchainFormat, 0, 1
            ))
                return 0;
        }
        
        // If the swapchain has been created and the images have been acquired, swapchain creation is succesful
        return 1;
    }

    uint8_t FindSwapchainSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info, 
    VkFormat& swapchainFormat)
    {
        // Get the amount of available surface formats
        uint32_t surfaceFormatsCount = 0; 
        if(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, 
        &surfaceFormatsCount, nullptr) != VK_SUCCESS)
            return 0;       
        // Pass the formats to a dynamic array
        BlitCL::DynamicArray<VkSurfaceFormatKHR> surfaceFormats(static_cast<size_t>(surfaceFormatsCount));
        if(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, 
        &surfaceFormatsCount, surfaceFormats.Data()) != VK_SUCCESS)
            return 0;
        // Look for the desired image format
        uint8_t found = 0;
        for(const auto& formats : surfaceFormats)
        {
            // If the desired image format is found, assigns it to the swapchain info and breaks out of the loop
            if(formats.format == VK_FORMAT_B8G8R8A8_UNORM && formats.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
                info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                // Saves the format to init handles
                swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
                found = 1;
                break;
            }
        }

        // If the desired format is not found (unlikely), assigns the first one that is supported and hopes for the best
        // I could have the function fail but this is not important enough and this function can be called at run time, so...
        if(!found)
        {
            info.imageFormat = surfaceFormats[0].format;
            info.imageColorSpace = surfaceFormats[0].colorSpace;
            // Save the image format
            swapchainFormat = info.imageFormat;
        }

        return 1;
    }

    uint8_t FindSwapchainPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info)
    {
        // Retrieves the amount of presentation modes supported
        uint32_t presentModeCount = 0;
        if(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, 
        &presentModeCount, nullptr) != VK_SUCCESS)
            return 0;

        // Saves the supported present modes to a dynamic array
        BlitCL::DynamicArray<VkPresentModeKHR> presentModes(static_cast<size_t>(presentModeCount));
        if(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, 
        &presentModeCount, presentModes.Data()) != VK_SUCCESS)
            return 0;

        // Looks for the desired presentation mode in the array
        uint8_t found = 0;
        for(const auto& present : presentModes)
        {
            // If the desired presentation mode is found, set the swapchain info to that
            if(present == ce_desiredPresentMode)
            {
                info.presentMode = ce_desiredPresentMode;
                found = 1;
                break;
            }
        }
                
        // If it was not found, sets it to this random smuck (this is supposed to be supported by everything and it's VSynced)
        if(!found)
            info.presentMode = VK_PRESENT_MODE_FIFO_KHR;

        return 1;
    }

    uint8_t FindSwapchainSurfaceCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info, 
    Swapchain& newSwapchain)
    {
        // Retrieves surface capabilities to properly configure some swapchain values
        VkSurfaceCapabilitiesKHR surfaceCapabilities{};
        if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS)
            return 0;

        // Updates the swapchain extent to the current extent from the surface, if it is not a special value
        if(surfaceCapabilities.currentExtent.width != UINT32_MAX)
        {
            newSwapchain.swapchainExtent = surfaceCapabilities.currentExtent;
        }

        // Gets the min extent and max extent allowed by the GPU,  to clamp the initial value
        VkExtent2D minExtent = surfaceCapabilities.minImageExtent;
        VkExtent2D maxExtent = surfaceCapabilities.maxImageExtent;

        newSwapchain.swapchainExtent.width = 
        BlitML::Clamp(newSwapchain.swapchainExtent.width, maxExtent.width, minExtent.width);

        newSwapchain.swapchainExtent.height = 
        BlitML::Clamp(newSwapchain.swapchainExtent.height, maxExtent.height, minExtent.height);

        // Swapchain extent fully checked and ready to pass to the swapchain info struct
        info.imageExtent = newSwapchain.swapchainExtent;

        uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
        // Checks that image count does not supass max image count
        if(surfaceCapabilities.maxImageCount > 0 && surfaceCapabilities.maxImageCount < imageCount )
        {
            imageCount = surfaceCapabilities.maxImageCount;
        }

        // Swapchain image count fully checked and ready to be pass to the swapchain info
        info.minImageCount = imageCount;

        info.preTransform = surfaceCapabilities.currentTransform;

        return 1;
    }

    uint8_t VulkanRenderer::CreateIdleDrawHandles()
    {
        VkDescriptorSetLayoutBinding backgroundImageLayoutBinding{};
        CreateDescriptorSetLayoutBinding(backgroundImageLayoutBinding, 0,
            1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT
        );
        m_backgroundImageSetLayout.handle = CreateDescriptorSetLayout(m_device,
            1, &backgroundImageLayoutBinding, // storage image binding
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR // uses push descriptors
        );
        if (m_backgroundImageSetLayout.handle == VK_NULL_HANDLE)
            return 0;

        // Creates the layout for the background compute shader
        VkPushConstantRange backgroundImageShaderPushConstant{};
        CreatePushConstantRange(
            backgroundImageShaderPushConstant, VK_SHADER_STAGE_COMPUTE_BIT,
            sizeof(BackgroundShaderPushConstant)
        );
        if (!CreatePipelineLayout(
            m_device, &m_basicBackgroundLayout.handle,
            1, &m_backgroundImageSetLayout.handle, // Image push descriptor layout
            1, &backgroundImageShaderPushConstant // Push constant for use data
        ))
            return 0;

        // Create the background shader in case the renderer has not objects
        if (!CreateComputeShaderProgram(m_device, "VulkanShaders/BasicBackground.comp.glsl.spv", // filepath
            VK_SHADER_STAGE_COMPUTE_BIT, "main", // entry point
            m_basicBackgroundLayout.handle, &m_basicBackgroundPipeline.handle
        ))
        {
            BLIT_ERROR("Failed to create BasicBackground.comp shader program")
                return 0;
        }

        VkCommandPoolCreateInfo idleCommandPoolInfo{};
        idleCommandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        idleCommandPoolInfo.pNext = nullptr;
        idleCommandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        idleCommandPoolInfo.queueFamilyIndex = m_graphicsQueue.index;
        if (vkCreateCommandPool(m_device, &idleCommandPoolInfo, nullptr,
            &m_idleCommandBufferPool.handle) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create idle draw command buffer pool")
                return 0;
        }
        VkCommandBufferAllocateInfo idleCommandBufferInfo{};
        idleCommandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        idleCommandBufferInfo.pNext = nullptr;
        idleCommandBufferInfo.commandBufferCount = 1;
        idleCommandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        idleCommandBufferInfo.commandPool = m_idleCommandBufferPool.handle;
        if (vkAllocateCommandBuffers(m_device, &idleCommandBufferInfo, &m_idleDrawCommandBuffer) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to allocate idle draw command buffer")
                return 0;
        }

		if (!CreateLoadingTrianglePipeline())
		{
			BLIT_ERROR("Failed to create loading triangle pipeline")
				return 0;
		}

        return 1;
    }

    // Few manual destructions remaining, mostly because of my laziness
    VulkanRenderer::~VulkanRenderer()
    {
        // Wait for the device to finish its work before destroying resources
        vkDeviceWaitIdle(m_device);

        for(size_t i = 0; i < m_depthPyramidMipLevels; ++i)
        {
            vkDestroyImageView(m_device, m_depthPyramidMips[i], m_pCustomAllocator);
        }

        if (m_debugMessenger != VK_NULL_HANDLE)
        {
            DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, m_pCustomAllocator);
        }
    }
}