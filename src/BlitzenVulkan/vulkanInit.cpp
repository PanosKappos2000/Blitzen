#include "vulkanRenderer.h"
#include "Platform/platform.h"
#include <cstring> // For strcmp

namespace BlitzenVulkan
{
    // Platform specific expressions
    #if _MSC_VER
        constexpr char* ce_surfaceExtensionName  = "VK_KHR_win32_surface";
        constexpr char* ce_baseValidationLayerName =  "VK_LAYER_KHRONOS_validation";                 
    #elif linux
        constexpr char* ce_surfaceExtensionName = "VK_KHR_xcb_surface";      
        constexpr char* ce_baseValidationLayerName = "VK_LAYER_NV_optimus";                  
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
            BLIT_INFO("Validation layer: %s", pCallbackData->pMessage)
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
        if(!BlitzenPlatform::CreateVulkanSurface(m_instance, m_surface, m_pCustomAllocator))
        {
            BLIT_ERROR("Failed to create Vulkan window surface")
            return 0;
        }

        // Call the function to search for a suitable physical device, it it can't find one return 0
        if(!PickPhysicalDevice(m_physicalDevice, m_instance, m_surface, 
        m_graphicsQueue, m_computeQueue, m_presentQueue, m_stats))
        {
            BLIT_ERROR("Failed to pick suitable physical device")
            return 0;
        }

        // Create the device
        if(!CreateDevice(m_device, m_physicalDevice, m_graphicsQueue, m_presentQueue, m_computeQueue, m_stats))
        {
            BLIT_ERROR("Failed to pick suitable physical device")
        }

        // Creates the swapchain
        if(!CreateSwapchain(m_device, m_surface, m_physicalDevice, 
        windowWidth, windowHeight, m_graphicsQueue, m_presentQueue, m_computeQueue, 
        m_pCustomAllocator, m_swapchainValues))
        {
            BLIT_ERROR("Failed to create Vulkan swapchain")
            return 0;
        }

        // This will be referred to by rendering attachments and will be updated when the window is resized
        m_drawExtent = {windowWidth, windowHeight};

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
        uint32_t extensionsCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
        BlitCL::DynamicArray<VkExtensionProperties> availableExtensions(static_cast<size_t>(extensionsCount));
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableExtensions.Data());
        uint8_t extensionSupport[ce_extensionCount] = {0};
        for(size_t i = 0; i < availableExtensions.GetSize(); ++i)
        {
            // Check for surafce extension support
            if(!extensionSupport[0] && !strcmp(availableExtensions[i].extensionName,ce_surfaceExtensionName))
            {
                extensionSupport[0] = 1;
            }
            if(!extensionSupport[1] && !strcmp(availableExtensions[i].extensionName, "VK_KHR_surface"))
            {
                extensionSupport[1] = 1;
            }

            // Check for validation layer extension support if the validation layers are active
            #if BLITZEN_VULKAN_VALIDATION_LAYERS
                if(!extensionSupport[ce_extensionCount - 1] &&
                !strcmp(availableExtensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
                {
                    extensionSupport[ce_extensionCount - 1] = 1;
                }
            #endif
        }

        // Checks that all of the required extensions are supported
        uint8_t allExtensions = 0;
        for(uint8_t i = 0; i < ce_extensionCount; ++i)
        {
            allExtensions += extensionSupport[i];
        }
        if(allExtensions != ce_extensionCount)
        {
            BLIT_ERROR("Not all extensions are supported")
            return 0;
        }

        // Creating an array of required extension names to pass to ppEnabledExtensionNames
        const char* requiredExtensionNames [ce_extensionCount];
        requiredExtensionNames[0] =  ce_surfaceExtensionName;
        requiredExtensionNames[1] = "VK_KHR_surface";        
        instanceInfo.enabledExtensionCount = ce_extensionCount;
        instanceInfo.enabledLayerCount = 0; // Validation layers inactive at first, but will be activated if it's a debug build

        // If validation layers are requested, the debut utils extension is also needed
        #if BLITZEN_VULKAN_VALIDATION_LAYERS
            // Adds the validation layer extensions to the extensions list
            requiredExtensionNames[ce_extensionCount - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

            uint8_t validationLayersEnabled = 0;
            const char* layerNameRef[2] = { ce_baseValidationLayerName, "VK_LAYER_KHRONOS_synchronization2" };
            VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
            if(EnableInstanceValidation(debugMessengerInfo))
            {
                // The debug messenger needs to be referenced by the instance
                instanceInfo.pNext = &debugMessengerInfo;

                // If the layer for synchronization 2 is found enable that as well
                if (EnabledInstanceSynchronizationValidation())
                    instanceInfo.enabledLayerCount = 2;
                // If not just enables the other one
                else
                    instanceInfo.enabledLayerCount = 1;


                instanceInfo.ppEnabledLayerNames = layerNameRef;
                validationLayersEnabled = 1;
            }

        #endif

        instanceInfo.ppEnabledExtensionNames = requiredExtensionNames;

        VkResult res = vkCreateInstance(&instanceInfo, nullptr, &instance);
        if(res != VK_SUCCESS)
            return 0;

        // If validation layers are request and they were succesfully found in the VkInstance earlier, the debug messenger is created
        #if BLITZEN_VULKAN_VALIDATION_LAYERS
            if(validationLayersEnabled)
                CreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, pDM);
        #endif

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
            BLIT_ERROR("The vulkan renderer will not be used in debug mode without validation layers")
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
    Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, 
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

        // Goes through the available devices, to eliminate the ones that are completely inadequate
        for(size_t i = 0; i < physicalDevices.GetSize(); ++i)
        {
            VkPhysicalDevice& pdv = physicalDevices[i];

            // Get core physical device features
            VkPhysicalDeviceFeatures features{};
            vkGetPhysicalDeviceFeatures(pdv, &features);

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
            !features11.storageBuffer16BitAccess || !features11.shaderDrawParameters ||
            !features12.bufferDeviceAddress || !features12.descriptorIndexing || !features12.runtimeDescriptorArray ||  
            !features12.storageBuffer8BitAccess || !features12.shaderFloat16 || !features12.drawIndirectCount ||
            !features12.samplerFilterMinmax || !features12.shaderInt8 || !features12.shaderSampledImageArrayNonUniformIndexing ||
            !features12.uniformAndStorageBuffer8BitAccess || !features12.storagePushConstant8 ||
            !features13.synchronization2 || !features13.dynamicRendering || !features13.maintenance4)
            {
                physicalDevices.RemoveAtIndex(i);
                --i;
                continue;
            }

            // Checking if the device supports all extensions that will be requested from Vulkan
            uint32_t dvExtensionCount = 0;
            vkEnumerateDeviceExtensionProperties(pdv, nullptr, &dvExtensionCount, nullptr);
            BlitCL::DynamicArray<VkExtensionProperties> dvExtensionsProps(static_cast<size_t>(dvExtensionCount));
            vkEnumerateDeviceExtensionProperties(pdv, nullptr, &dvExtensionCount, dvExtensionsProps.Data());
            // For now the device only needs to look for one extension
            uint8_t extensionSupport  = 0;
            // Check for the required extension name with strcmp
            for(size_t j = 0; j < dvExtensionsProps.GetSize(); ++j)
            {
                if(!strcmp(dvExtensionsProps[j].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME))
                    extensionSupport = 1;  
            }
                
            if(!extensionSupport)
            {
                physicalDevices.RemoveAtIndex(i);
                --i;
                continue;
            }

            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(pdv, &props);
            if (props.apiVersion < VK_API_VERSION_1_3)
            {
                physicalDevices.RemoveAtIndex(i);
                --i;
                continue;
            }

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
            BlitCL::DynamicArray<VkQueueFamilyProperties2> queueFamilyProperties(
            static_cast<size_t>(queueFamilyPropertyCount), std::move(VkQueueFamilyProperties2({})));
            for(size_t j = 0; j < queueFamilyProperties.GetSize(); ++j)
            {
                queueFamilyProperties[j].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
            }
            vkGetPhysicalDeviceQueueFamilyProperties2(pdv, &queueFamilyPropertyCount, queueFamilyProperties.Data());
            for(size_t j = 0; j < queueFamilyProperties.GetSize(); ++j)
            {
                // Checks for a graphics queue index, if one has not already been found 
                if(queueFamilyProperties[j].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT && !graphicsQueue.hasIndex)
                {
                    graphicsQueue.index = static_cast<uint32_t>(j);
                    graphicsQueue.hasIndex = 1;
                }

                // Checks for a compute queue index, if one has not already been found 
                if(queueFamilyProperties[j].queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT && !computeQueue.hasIndex)
                {
                    computeQueue.index = static_cast<uint32_t>(j);
                    computeQueue.hasIndex = 1;
                }

                VkBool32 supportsPresent = VK_FALSE;
                VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(pdv, static_cast<uint32_t>(j), surface, &supportsPresent))
                if(supportsPresent == VK_TRUE && !presentQueue.hasIndex)
                {
                    presentQueue.index = static_cast<uint32_t>(j);
                    presentQueue.hasIndex = 1;
                }
            }

            // If one of the required queue families has no index, then it gets removed from the candidates
            if(!presentQueue.hasIndex || !graphicsQueue.hasIndex || !computeQueue.hasIndex)
            {
                physicalDevices.RemoveAtIndex(i);
                --i;
            }
        }

        if(!physicalDevices.GetSize())
        {
            BLIT_WARN("Your machine has no physical device that supports vulkan the way Blitzen wants it. \n \
            Try another graphics API")
            return 0;
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
                gpu = pdv;
                stats.hasDiscreteGPU = 1;
            }
        }

        // If a discrete GPU is not found, the renderer chooses the 1st device. This will change the way the renderer goes forward
        if(!stats.hasDiscreteGPU)
            gpu = physicalDevices[0];
        else
            BLIT_INFO("Discrete GPU found")

        // If the function has reached this point, it means it found a physical device
        return 1;
    }

    uint8_t CreateDevice(VkDevice& device, VkPhysicalDevice physicalDevice, Queue& graphicsQueue, 
    Queue& presentQueue, Queue& computeQueue, VulkanStats& stats)
    {
        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.flags = 0; // Not using this
        deviceInfo.enabledLayerCount = 0;//Deprecated

        // Since mesh shaders are not a required feature, they will be checked separately and if support is not found, the traditional pipeline will be used
        #ifdef BLIT_VK_MESH_EXT
            VkPhysicalDeviceFeatures2 features2{};
            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            VkPhysicalDeviceMeshShaderFeaturesEXT meshFeatures{};
            meshFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
            features2.pNext = &meshFeatures;
            vkGetPhysicalDeviceFeatures2(initHandles.chosenGpu, &features2);
            stats.meshShaderSupport = meshFeatures.meshShader && meshFeatures.taskShader;

            // Check the extensions as well
            uint32_t dvExtensionCount = 0;
            vkEnumerateDeviceExtensionProperties(initHandles.chosenGpu, nullptr, &dvExtensionCount, nullptr);
            BlitCL::DynamicArray<VkExtensionProperties> dvExtensionsProps(static_cast<size_t>(dvExtensionCount));
            vkEnumerateDeviceExtensionProperties(initHandles.chosenGpu, nullptr, &dvExtensionCount, dvExtensionsProps.Data());
            uint8_t meshShaderExtension = 0;
            for(size_t i = 0; i < dvExtensionsProps.GetSize(); ++i)
            {
                if(!strcmp(dvExtensionsProps[i].extensionName, VK_EXT_MESH_SHADER_EXTENSION_NAME))
                    meshShaderExtension = 1;
            }
            stats.meshShaderSupport = stats.meshShaderSupport && meshShaderExtension;

            if(stats.meshShaderSupport)
                BLIT_INFO("Mesh shader support confirmed")
            else
                BLIT_INFO("No mesh shader support, using traditional pipeline")
        #endif

        // Vulkan should ignore the mesh shader extension if support for it was not found
        deviceInfo.enabledExtensionCount = 2 + (ce_bMeshShaders && stats.meshShaderSupport);
        // Adding the swapchain extension and mesh shader extension if it was requested
        const char* extensionsNames[2 + ce_bMeshShaders];
        extensionsNames[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        extensionsNames[1] = VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME;
        #if BLITZEN_VULKAN_MESH_SHADER
            if(stats.meshShaderSupport)
                extensionsNames[2] = VK_EXT_MESH_SHADER_EXTENSION_NAME;
        #endif
        deviceInfo.ppEnabledExtensionNames = extensionsNames;

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
        #if BLITZEN_VULKAN_MESH_SHADER
            if(stats.meshShaderSupport)
            {
                vulkanFeaturesMesh.meshShader = true;
                vulkanFeaturesMesh.taskShader = true;
            }
        #endif

        VkPhysicalDeviceFeatures2 vulkanExtendedFeatures{};
        vulkanExtendedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        vulkanExtendedFeatures.features = deviceFeatures;

        // Add all features structs to the pNext chain
        deviceInfo.pNext = &vulkanExtendedFeatures;
        vulkanExtendedFeatures.pNext = &vulkan11Features;
        vulkan11Features.pNext = &vulkan12Features;
        vulkan12Features.pNext = &vulkan13Features;
        vulkan13Features.pNext = &vulkanFeaturesMesh;

        BlitCL::DynamicArray<VkDeviceQueueCreateInfo> queueInfos(1);
        queueInfos[0].queueFamilyIndex = graphicsQueue.index;
        VkDeviceQueueCreateInfo deviceQueueInfo{};
        // If compute has a different index from present, add a new info for it
        if(graphicsQueue.index != computeQueue.index)
        {
            queueInfos.PushBack(deviceQueueInfo);
            queueInfos[1].queueFamilyIndex = computeQueue.index;
        }
        // If an info was created for compute and present is not equal to compute or graphics, create a new one for present as well
        if(queueInfos.GetSize() == 2 && queueInfos[0].queueFamilyIndex != presentQueue.index && queueInfos[1].queueFamilyIndex != presentQueue.index)
        {
            queueInfos.PushBack(deviceQueueInfo);
            queueInfos[2].queueFamilyIndex = presentQueue.index;
        }
        // If an info was not created for compute but present has a different index from the other 2, create a new info for it
        if(queueInfos.GetSize() == 1 && queueInfos[0].queueFamilyIndex != presentQueue.index)
        {
            queueInfos.PushBack(deviceQueueInfo);
            queueInfos[1].queueFamilyIndex = presentQueue.index;
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
            swapchainInfo.clipped = VK_TRUE;// Don't present things renderer out of bounds
            swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;// Might not be supported in some cases, so I might want to guard against this
            swapchainInfo.surface = surface;
            swapchainInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            // Used when the swapchain is recreated
            swapchainInfo.oldSwapchain = oldSwapchain;

            /* Image format and color space */
            {
                // Get the amount of available surface formats
                uint32_t surfaceFormatsCount = 0; 
                if(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, 
                &surfaceFormatsCount, nullptr) != VK_SUCCESS)
                    // Something is seriously wrong if the above function does not work
                    return 0;
                
                // Pass the formats to a dynamic array
                BlitCL::DynamicArray<VkSurfaceFormatKHR> surfaceFormats(static_cast<size_t>(surfaceFormatsCount));
                if(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, 
                &surfaceFormatsCount, surfaceFormats.Data()) != VK_SUCCESS)
                    // Something is seriously wrong if the above function does not work
                    return 0;

                // Look for the desired image format
                uint8_t found = 0;
                for(size_t i = 0; i < surfaceFormats.GetSize(); ++i)
                {
                    // If the desired image format is found, assigns it to the swapchain info and breaks out of the loop
                    if(surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM && 
                    surfaceFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                    {
                        swapchainInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
                        swapchainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                        // Saves the format to init handles
                        newSwapchain.swapchainFormat = VK_FORMAT_B8G8R8A8_UNORM;
                        found = 1;
                        break;
                    }
                }

                // If the desired format is not found (unlikely), assigns the first one that is supported and hopes for the best
                // I could have the function fail but this is not important enough and this function can be called at run time, so...
                if(!found)
                {
                    swapchainInfo.imageFormat = surfaceFormats[0].format;
                    // Save the image format
                    newSwapchain.swapchainFormat = swapchainInfo.imageFormat;
                    swapchainInfo.imageColorSpace = surfaceFormats[0].colorSpace;
                }
            }

            /* Present mode */
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
                for(size_t i = 0; i < presentModes.GetSize(); ++i)
                {
                    // If the desired presentation mode is found, set the swapchain info to that
                    if(presentModes[i] == ce_desiredPresentMode)
                    {
                        swapchainInfo.presentMode = ce_desiredPresentMode;
                        found = 1;
                        break;
                    }
                }
                
                // If it was not found, sets it to this random smuck (this is supposed to be supported by everything and it's VSynced)
                if(!found)
                {
                    swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
                }
            }

            // Sets the swapchain extent to the window's width and height
            newSwapchain.swapchainExtent = {windowWidth, windowHeight};
            // Retrieves surface capabilities to properly configure some swapchain values
            VkSurfaceCapabilitiesKHR surfaceCapabilities{};
            if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS)
                return 0;

            /* Swapchain extent */
            {
                // Updates the swapchain extent to the current extent from the surface, if it is not a special value
                if(surfaceCapabilities.currentExtent.width != UINT32_MAX)
                {
                    newSwapchain.swapchainExtent = surfaceCapabilities.currentExtent;
                }

                // Gets the min extent and max extent allowed by the GPU,  to clamp the initial value
                VkExtent2D minExtent = surfaceCapabilities.minImageExtent;
                VkExtent2D maxExtent = surfaceCapabilities.maxImageExtent;

                newSwapchain.swapchainExtent.width = 
                BlitCL::Clamp(newSwapchain.swapchainExtent.width, maxExtent.width, minExtent.width);

                newSwapchain.swapchainExtent.height = 
                BlitCL::Clamp(newSwapchain.swapchainExtent.height, maxExtent.height, minExtent.height);

                // Swapchain extent fully checked and ready to pass to the swapchain info struct
                swapchainInfo.imageExtent = newSwapchain.swapchainExtent;
            }

            /* Min image count */
            {
                uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
                // Checks that image count does not supass max image count
                if(surfaceCapabilities.maxImageCount > 0 && surfaceCapabilities.maxImageCount < imageCount )
                {
                    imageCount = surfaceCapabilities.maxImageCount;
                }

                // Swapchain image count fully checked and ready to be pass to the swapchain info
                swapchainInfo.minImageCount = imageCount;
            }

            swapchainInfo.preTransform = surfaceCapabilities.currentTransform;

            // Configure queue settings based on if the graphics queue also supports presentation
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
            VkResult swapchainResult = vkCreateSwapchainKHR(device, &swapchainInfo, pCustomAllocator, 
            &newSwapchain.swapchainHandle);
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
            return 1;
    }


    void VulkanRenderer::Shutdown()
    {
        // Wait for the device to finish its work before destroying resources
        vkDeviceWaitIdle(m_device);

        vkDestroySampler(m_device, m_placeholderSampler, m_pCustomAllocator);

        // Destroys the resources used for the texture descriptors
        vkDestroyDescriptorPool(m_device, m_textureDescriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_textureDescriptorSetlayout, nullptr);

        vkDestroyDescriptorSetLayout(m_device, m_pushDescriptorBufferLayout, m_pCustomAllocator);

        vkDestroyPipeline(m_device, m_lateDrawCullPipeline, m_pCustomAllocator);
        vkDestroyPipelineLayout(m_device, m_drawCullPipelineLayout, m_pCustomAllocator);
        vkDestroyPipeline(m_device, m_initialDrawCullPipeline, m_pCustomAllocator);

        vkDestroyPipeline(m_device, m_opaqueGeometryPipeline, m_pCustomAllocator);
        vkDestroyPipeline(m_device, m_postPassGeometryPipeline, m_pCustomAllocator);
        vkDestroyPipelineLayout(m_device, m_opaqueGeometryPipelineLayout, m_pCustomAllocator);

        vkDestroyPipeline(m_device, m_depthPyramidGenerationPipeline, m_pCustomAllocator);
        vkDestroyPipelineLayout(m_device, m_depthPyramidGenerationPipelineLayout, m_pCustomAllocator);
        vkDestroyDescriptorSetLayout(m_device, m_depthPyramidDescriptorLayout, m_pCustomAllocator);

        for(size_t i = 0; i < m_depthPyramidMipLevels; ++i)
        {
            vkDestroyImageView(m_device, m_depthPyramidMips[i], m_pCustomAllocator);
        }
        vkDestroySampler(m_device, m_depthAttachmentSampler, m_pCustomAllocator);

        for(size_t i = 0; i < ce_framesInFlight; ++i)
        {
            FrameTools& frameTools = m_frameToolsList[i];
            VarBuffers& varBuffers = m_varBuffers[i];

            vkDestroyCommandPool(m_device, frameTools.mainCommandPool, m_pCustomAllocator);

            vkDestroyFence(m_device, frameTools.inFlightFence, m_pCustomAllocator);
            vkDestroySemaphore(m_device, frameTools.imageAcquiredSemaphore, m_pCustomAllocator);
            vkDestroySemaphore(m_device, frameTools.readyToPresentSemaphore, m_pCustomAllocator);
        }

        vkDestroySwapchainKHR(m_device, m_swapchainValues.swapchainHandle, m_pCustomAllocator);

        DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, m_pCustomAllocator);

        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    }

    VulkanRenderer::~VulkanRenderer()
    {
        
    }
}