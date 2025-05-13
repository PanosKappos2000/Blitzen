#include "vulkanRenderer.h"
#include "vulkanCommands.h"
#include "Platform/platform.h"
#include "vulkanResourceFunctions.h"
#include "vulkanPipelines.h"
#include <cstring> // For strcmp

namespace BlitzenVulkan
{
    VulkanRenderer* VulkanRenderer::m_pThisRenderer = nullptr;

    VulkanRenderer::VulkanRenderer() :
        m_pCustomAllocator{ nullptr }, m_debugMessenger{ VK_NULL_HANDLE },
        m_currentFrame{ 0 }, m_loadingTriangleVertexColor{ 0.1f, 0.8f, 0.3f },
        m_depthPyramidMipLevels{ 0 }, textureCount{ 0 }
    {
        m_pThisRenderer = this;
    }

    static void CreateApplicationInfo(VkApplicationInfo& appInfo, void* pNext, const char* appName, uint32_t appVersion,
        const char* engineName, uint32_t engineVersion, uint32_t apiVersion = VK_API_VERSION_1_3)
    {
        appInfo = {};

        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pNext = nullptr;
        appInfo.apiVersion = VK_API_VERSION_1_3;

        appInfo.pApplicationName = appName;
        appInfo.applicationVersion = appVersion;
        appInfo.pEngineName = engineName;
        appInfo.engineVersion = engineVersion;
    }

    struct InstanceExtensionContext
    {
        BlitCL::DynamicArray<VkExtensionProperties> availableExtensions;

        const char* possibleRequestedExtensions[Ce_MaxRequestedInstanceExtensions]{ ce_surfaceExtensionName, "VK_KHR_surface", VK_EXT_DEBUG_UTILS_EXTENSION_NAME };
        uint8_t extensionsSupportRequested[Ce_MaxRequestedInstanceExtensions]{ Ce_SurfaceExtensionsRequired, Ce_SurfaceExtensionsRequired, ce_bValidationLayersRequested || Ce_ValidationExtensionRequired };
        uint8_t extensionSupportRequired[Ce_MaxRequestedInstanceExtensions]{ Ce_SurfaceExtensionsRequired, Ce_SurfaceExtensionsRequired, Ce_ValidationExtensionRequired };

        const char* extensionNames[Ce_MaxRequestedInstanceExtensions]{};
        uint8_t extensionSupportFound[Ce_MaxRequestedInstanceExtensions]{ 0, 0, 0 };
    };
    static uint8_t FindInstanceExtensions(VkInstanceCreateInfo& instanceInfo, InstanceExtensionContext& ctx)
    {
        uint32_t extensionCount = 0;
        for (size_t i = 0; i < Ce_MaxRequestedInstanceExtensions; ++i)
        {
            if (!ctx.extensionsSupportRequested[i])
            {
                continue;
            }

            for (auto& extension : ctx.availableExtensions)
            {
                // Check for surafce extension support
                if (!strcmp(extension.extensionName, ctx.possibleRequestedExtensions[i]))
                {
                    ctx.extensionNames[extensionCount++] = extension.extensionName;
                    ctx.extensionSupportFound[i] = 1;
                }
            }

            if (!ctx.extensionSupportFound[i] && ctx.extensionSupportRequired[i])
            {
                BLIT_ERROR("Vulkan instance exetension with name: %s was not found", ctx.possibleRequestedExtensions[i]);
                return 0;
            }

            if (!ctx.extensionSupportFound[i] && !ctx.extensionSupportRequired[i])
            {
                BLIT_WARN("Vulkan instance exetension with name: %s was not found", ctx.possibleRequestedExtensions[i]);
            }

        }

        instanceInfo.ppEnabledExtensionNames = ctx.extensionNames;
        instanceInfo.enabledExtensionCount = extensionCount;

        return 1;
    }

    static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr)
        {
            return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
        }

        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
    {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
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
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        {
            BLIT_INFO("Validation layer: %s", pCallbackData->pMessage);
            return VK_FALSE;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        {
            BLIT_WARN("Validation layer: %s", pCallbackData->pMessage);
            return VK_FALSE;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        {
            BLIT_ERROR("Validation layer: %s", pCallbackData->pMessage);
            return VK_FALSE;
        }
        default:
        {
            return VK_FALSE;
        }
        }
    }

    static uint8_t EnableInstanceValidation(VkDebugUtilsMessengerCreateInfoEXT& debugMessengerInfo)
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
            if (!strcmp(availableLayers[i].layerName, ce_baseValidationLayerName))
            {
                layersFound = 1;
                break;
            }
        }

        if (!layersFound)
        {
            BLIT_ERROR("The vulkan renderer should not be used in debug mode without validation layers");
            return 0;
        }

        // Create the debug messenger
        debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

        debugMessengerInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;

        debugMessengerInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

        // Debug messenger callback function defined at the top of this file
        debugMessengerInfo.pfnUserCallback = debugCallback;

        debugMessengerInfo.pNext = nullptr;
        debugMessengerInfo.pUserData = nullptr;

        return 1;
    }

    static uint8_t EnabledInstanceSynchronizationValidation()
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

    static uint8_t EnableValidationLayers(VkInstanceCreateInfo& instanceInfo, VkDebugUtilsMessengerCreateInfoEXT& debugMessengerInfo, VkValidationFeaturesEXT& validationFeatures,
        VkValidationFeatureEnableEXT* pEnables, const char** ppEnabledLayerNames)
    {
        if (EnableInstanceValidation(debugMessengerInfo))
        {
            validationFeatures = {};
            validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
            validationFeatures.enabledValidationFeatureCount = 1;
            validationFeatures.pEnabledValidationFeatures = pEnables;
            validationFeatures.disabledValidationFeatureCount = 0;
            validationFeatures.pDisabledValidationFeatures = nullptr;
            validationFeatures.pNext = &debugMessengerInfo;

            instanceInfo.pNext = &validationFeatures;

            // If the layer for synchronization 2 is found, it enables that as well
            if (ce_bSynchronizationValidationRequested && EnabledInstanceSynchronizationValidation())
            {
                instanceInfo.enabledLayerCount = 2;
            }
            else
            {
                BLIT_WARN("Failed to enable Vulkan synchronization validation");
                instanceInfo.enabledLayerCount = 1;
            }

            instanceInfo.ppEnabledLayerNames = ppEnabledLayerNames;

            return 1;
        }

        BLIT_ERROR("Failed to enable validation layers");
        return 0;
    }

    static uint8_t CreateInstance(VkInstance& instance, VkDebugUtilsMessengerEXT* pDM = nullptr)
    {
        uint32_t apiVersion = 0;
        VK_CHECK(vkEnumerateInstanceVersion(&apiVersion));
        if (apiVersion < Ce_VkApiVersion)
        {
            BLIT_ERROR("Required Vulkan API version not supported");
            return 0;
        }

        const char* appName{ BlitzenEngine::Ce_HostedApp };
        const char* engineName{ BlitzenEngine::ce_blitzenVersion };
        VkApplicationInfo applicationInfo{};
        CreateApplicationInfo(applicationInfo, nullptr, appName, VK_MAKE_VERSION(BlitzenEngine::Ce_HostedAppVersion, 0, 0), engineName, VK_MAKE_VERSION(BlitzenEngine::ce_blitzenMajor, 0, 0));

        VkInstanceCreateInfo instanceInfo{};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pNext = nullptr;
        instanceInfo.flags = 0;
        instanceInfo.pApplicationInfo = &applicationInfo;

        // Enumerates
        uint32_t availableExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
        // Initialize data
        InstanceExtensionContext extensionContext;
        extensionContext.availableExtensions.Resize(availableExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, extensionContext.availableExtensions.Data());
        // Finds extensions
        if (!FindInstanceExtensions(instanceInfo, extensionContext))
        {
            BLIT_ERROR("Failed to find all required instance extensions");
            return 0;
        }

        instanceInfo.enabledLayerCount = 0;
        uint8_t validationLayersEnabled = 0;
        VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};
        // Shader printf
        VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };
        VkValidationFeaturesEXT validationFeatures{};
        const char* layerNames[2] = { ce_baseValidationLayerName, Ce_SyncValidationLayerName };
        if (ce_bValidationLayersRequested && extensionContext.extensionSupportFound[Ce_ValidationExtensionElement])
        {
            validationLayersEnabled = EnableValidationLayers(instanceInfo, debugMessengerInfo, validationFeatures, enables, layerNames);
        }

        auto res = vkCreateInstance(&instanceInfo, nullptr, &instance);
        if (res != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create instance");
            return 0;
        }

        // If validation layers are request and they were succesfully found in the VkInstance earlier, the debug messenger is created
        if (validationLayersEnabled)
        {
            BLIT_INFO("Setting up Vulkan Debug Messenger");
            CreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, pDM);
        }

        return 1;
    }

    static uint8_t ValidatePdvFeatures(VkPhysicalDevice pdv)
    {
        VkPhysicalDeviceFeatures features{};
        vkGetPhysicalDeviceFeatures(pdv, &features);

        VkPhysicalDeviceFeatures2 features2{};
        features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        VkPhysicalDeviceVulkan11Features features11{};
        features11.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        features2.pNext = &features11;

        VkPhysicalDeviceVulkan12Features features12{};
        features12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        features11.pNext = &features12;

        VkPhysicalDeviceVulkan13Features features13{};
        features13.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        features12.pNext = &features13;

        VkPhysicalDeviceMeshShaderFeaturesEXT featuresMesh{};
        featuresMesh.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        features13.pNext = &featuresMesh;

        vkGetPhysicalDeviceFeatures2(pdv, &features2);

        // Check that all the required features are supported by the device
        if (!features.multiDrawIndirect || !features.samplerAnisotropy ||
            !features11.storageBuffer16BitAccess || !features11.shaderDrawParameters ||
            !features12.bufferDeviceAddress || !features12.descriptorIndexing || !features12.runtimeDescriptorArray || !features12.storageBuffer8BitAccess ||
            !features12.shaderFloat16 || !features12.drawIndirectCount || !features12.samplerFilterMinmax || !features12.shaderInt8 || 
            !features12.shaderSampledImageArrayNonUniformIndexing ||!features12.uniformAndStorageBuffer8BitAccess || !features12.storagePushConstant8 ||
            !features13.synchronization2 || !features13.dynamicRendering || !features13.maintenance4)
        {
            return 0;
        }

        return 1;
    }

    struct ExtensionQueryHelper
    {
        const char* extensionName;
        uint8_t bSupportRequested;
        uint8_t bSupportRequired;
        uint8_t bSupportFound;

        inline ExtensionQueryHelper(const char* name, uint8_t bRequested, uint8_t bRequired)
            :extensionName{ name }, bSupportRequested{ bRequested },
            bSupportRequired{ bRequired }, bSupportFound{ 0 }
        {}

    };
    static uint8_t LookForRequestedExtensions(VkPhysicalDevice pdv, VulkanStats& stats)
    {
        // Checking if the device supports all extensions that will be requested from Vulkan
        uint32_t dvExtensionCount = 0;
        vkEnumerateDeviceExtensionProperties(pdv, nullptr, &dvExtensionCount, nullptr);
        BlitCL::DynamicArray<VkExtensionProperties> dvExtensionsProps(static_cast<size_t>(dvExtensionCount));
        vkEnumerateDeviceExtensionProperties(pdv, nullptr, &dvExtensionCount, dvExtensionsProps.Data());

        ExtensionQueryHelper extensionsData[Ce_MaxRequestedDeviceExtensions]
        {
            { VK_KHR_SWAPCHAIN_EXTENSION_NAME, Ce_SwapchainExtensionRequested, Ce_SwapchainExtensionRequired },
            { VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, Ce_IsPushDescriptorExtensionsRequested, Ce_IsPushDescriptorExtensionsRequired },
            { VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, Ce_RayTracingRequested, Ce_RayTracingRequired },
            { VK_KHR_RAY_QUERY_EXTENSION_NAME, Ce_RayTracingRequested, Ce_RayTracingRequired },
            { VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, Ce_RayTracingRequested, Ce_RayTracingRequired },
            { VK_EXT_MESH_SHADER_EXTENSION_NAME, Ce_MeshShadersRequested, Ce_MeshShadersRequired },
            { VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME, ce_bSynchronizationValidationRequested, Ce_SyncValidationDeviceExtensionRequired},
            { VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME, Ce_GPUPrintfDeviceExtensionRequested, Ce_GPUPrintfDeviceExtensionRequired}
        };

        // Check for the required extension name with strcmp
        for (auto& data : extensionsData)
        {
            // If the extensions was never requested, it does not bother checking for it
            if (!data.bSupportRequested)
            {
                continue;
            }

            for (auto& extension : dvExtensionsProps)
            {
                if (!strcmp(extension.extensionName, data.extensionName))
                {
                    data.bSupportFound = 1;
                    stats.deviceExtensionNames[stats.deviceExtensionCount++] = data.extensionName;
                }
            }

            if (!data.bSupportFound && data.bSupportRequired)
            {
                BLIT_ERROR("Device extension with name: %s, not supported", data.extensionName);
                return 0;
            }
        }

        uint8_t syncValidationSupportFound = extensionsData[Ce_SyncValidationDeviceExtensionElement].bSupportFound;
        if (syncValidationSupportFound)
        {
            // Check for mesh shader feature in available features
            VkPhysicalDeviceFeatures2 features2{};
            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            VkPhysicalDeviceSynchronization2Features  syncFeatures{};
            syncFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
            features2.pNext = &syncFeatures;
            vkGetPhysicalDeviceFeatures2(pdv, &features2);

            if (syncFeatures.synchronization2)
            {
                BLIT_INFO("Sync validation support confirmed");
            }
            else
            {
                BLIT_ERROR("No sync validation support");
            }
        }

        // Check for mesh shaders features and extensions
        uint8_t meshShaderSupportFound = extensionsData[Ce_MeshShaderExtensionElement].bSupportFound;
        if (Ce_MeshShadersRequested)
        {
            // Check for mesh shader feature in available features
            VkPhysicalDeviceFeatures2 features2{};
            features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            VkPhysicalDeviceMeshShaderFeaturesEXT meshFeatures{};
            meshFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
            features2.pNext = &meshFeatures;
            vkGetPhysicalDeviceFeatures2(pdv, &features2);

            if (meshFeatures.meshShader && meshFeatures.taskShader)
            {
                BLIT_INFO("Mesh shader support confirmed");
            }
            else
            {
                BLIT_ERROR("No mesh shader support, using traditional pipeline");
            }
        }

        // Checks for raytracing extensions and features
        bool rayTracingSupportFound{ false };
        rayTracingSupportFound = rayTracingSupportFound && extensionsData[2].bSupportFound;
        rayTracingSupportFound = rayTracingSupportFound && extensionsData[3].bSupportFound; 
        rayTracingSupportFound = rayTracingSupportFound && extensionsData[4].bSupportFound;
        if (rayTracingSupportFound)
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

            if (rayQuery.rayQuery && ASfeats.accelerationStructure)
            {
                BLIT_INFO("Ray tracing support confirmed");
            }
            else
            {
                BLIT_ERROR("No ray tracing support found, using traditional raster");
            }
        }

        return 1;
    }

    static uint8_t ValidatePdvQueueFamilies(VkPhysicalDevice pdv, VkSurfaceKHR surface, 
        Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, Queue& transferQueue, VulkanStats& stats, 
        BlitCL::DynamicArray< VkQueueFamilyProperties2>& queueFamilyProperties)
    {
        uint32_t queueIndex = 0;
        // For the main graphics queue, find the first family with queue graphics bit set
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
            bool isComputeCapable = queueProps.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT;
            bool isGraphicsCapable = queueProps.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
            bool isTransferCapable = queueProps.queueFamilyProperties.queueFlags & VK_QUEUE_TRANSFER_BIT;

            bool isDedicatedTransfer = isTransferCapable && !isGraphicsCapable && !isComputeCapable;

            // Checks for a transfer queue index, if one has not already been found
            if (isDedicatedTransfer && queueIndex != graphicsQueue.index)
            {
                transferQueue.index = queueIndex;
                transferQueue.hasIndex = 1;
                BLIT_INFO("Found dedicated transfer queue");
                break;
            }

            ++queueIndex;
        }

        queueIndex = 0;
        // Searches for a dedicated compute queue
        for (auto& props : queueFamilyProperties)
        {
            bool isComputeCapable = props.queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT;
            bool isGraphicsCapable = props.queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT;

            bool isDedicatedCompute = isComputeCapable && !isGraphicsCapable;

            if (isDedicatedCompute && queueIndex != graphicsQueue.index && queueIndex != transferQueue.index)
            {
                computeQueue.index = queueIndex;
                computeQueue.hasIndex = 1;
                BLIT_INFO("Found dedicated compute queue");
                break;
            }

            ++queueIndex;
        }

        queueIndex = 0;
        for (auto& queueProps : queueFamilyProperties)
        {
            // Checks for presentation queue, if one was not already found
            VkBool32 supportsPresent = VK_FALSE;
            VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(pdv, queueIndex, surface, &supportsPresent));
            if (supportsPresent == VK_TRUE && !presentQueue.hasIndex)
            {
                presentQueue.index = queueIndex;
                presentQueue.hasIndex = 1;
                break;
            }
            ++queueIndex;
        }

        // If one of the required queue families has no index, then it gets removed from the candidates
        if (!presentQueue.hasIndex || !graphicsQueue.hasIndex || !transferQueue.hasIndex)
        {
            BLIT_ERROR("Failed to find all required queue families on physical device");
            return 0;
        }

        if (BlitzenEngine::Ce_BuildClusters && !computeQueue.hasIndex)
        {
            BLIT_ERROR("Vulkan Cluster mode needs dedicated compute queue");
            return 0;
        }

        return 1;
    }

    static uint8_t ValidatePhysicalDevice(VkPhysicalDevice pdv, VkInstance instance, VkSurfaceKHR surface,
        Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, Queue& transferQueue, VulkanStats& stats)
    {
        // Features and properties
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(pdv, &props);
        if (props.apiVersion < VK_API_VERSION_1_3)
        {
            BLIT_ERROR("Physical device api version does not match the requested Vulkan API version");
            return 0;
        }

        // Features
        if (!ValidatePdvFeatures(pdv))
        {
            BLIT_ERROR("Physical device does not support all features");
            return 0;
        }

        // Extensions
        if (!LookForRequestedExtensions(pdv, stats))
        {
            BLIT_ERROR("Physical device does not support all extensions");
            return 0;
        }

        // Enumerates
        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(pdv, &queueFamilyPropertyCount, nullptr);
        if (!queueFamilyPropertyCount)
        {
            BLIT_ERROR("Physical Device has No Queue Families (???)");
            return 0;
        }
        // Stores data
        BlitCL::DynamicArray<VkQueueFamilyProperties2> queueFamilyProperties(size_t(queueFamilyPropertyCount), std::move(VkQueueFamilyProperties2({})));// WTF???
        for (auto& queueProps : queueFamilyProperties)
        {
            queueProps.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        }
        vkGetPhysicalDeviceQueueFamilyProperties2(pdv, &queueFamilyPropertyCount, queueFamilyProperties.Data());

        // Queue families
        if(!ValidatePdvQueueFamilies(pdv, surface, graphicsQueue, computeQueue, presentQueue, transferQueue, stats, queueFamilyProperties))
        {
            return 0;
        }

        return 1;

    }

    static uint8_t PickPhysicalDevice(VkPhysicalDevice& gpu, VkInstance instance, VkSurfaceKHR surface,
        Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, Queue& transferQueue, VulkanStats& stats)
    {
        // Retrieves the physical device count
        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
        if (!physicalDeviceCount)
            return 0;

        // Pass the available devices to an array to pick the best one
        BlitCL::DynamicArray<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
        vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.Data());

        uint32_t extensionCount = 0;
        for (auto& pdv : physicalDevices)
        {
            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(pdv, &props);
            // Only choose discrete GPUs at first
            if (props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                continue;
            }
            // Checks if the discrete gpu is suitable
            if (ValidatePhysicalDevice(pdv, instance, surface, graphicsQueue, computeQueue, presentQueue, transferQueue, stats))
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
        for (auto& pdv : physicalDevices)
        {
            // Checks for possible non discrete GPUs
            if (!ValidatePhysicalDevice(pdv, instance, surface, graphicsQueue,
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

    struct DeviceFeaturesContext
    {
        VkPhysicalDeviceFeatures deviceFeatures{};
        VkPhysicalDeviceVulkan11Features vulkan11Features{};
        VkPhysicalDeviceVulkan12Features vulkan12Features{};
        VkPhysicalDeviceVulkan13Features vulkan13Features{};
        VkPhysicalDeviceMeshShaderFeaturesEXT vulkanFeaturesMesh{};
        VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures{};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures{};

        VkPhysicalDeviceFeatures2 vulkanExtendedFeatures{};
    };
    static void AddDeviceFeatures(VkDeviceCreateInfo& info, DeviceFeaturesContext& ctx, VulkanStats& stats)
    {
        // Allows the renderer to use one vkCmdDrawIndrect type call for multiple objects
        ctx.deviceFeatures.multiDrawIndirect = true;

        // Allows sampler anisotropy to be VK_TRUE when creating a VkSampler
        ctx.deviceFeatures.samplerAnisotropy = true;
        
        ctx.vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        ctx.vulkan11Features.shaderDrawParameters = true;
        ctx.vulkan11Features.storageBuffer16BitAccess = true;

        ctx.vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

        // Allow the application to get the address of a buffer and pass it to the shaders
        ctx.vulkan12Features.bufferDeviceAddress = true;

        // Allows the shaders to index into array held by descriptors, needed for textures
        ctx.vulkan12Features.descriptorIndexing = true;

        // Allows shaders to use array with undefined size for descriptors, needed for textures
        ctx.vulkan12Features.runtimeDescriptorArray = true;

        // Allows the use of float16_t type in the shaders
        ctx.vulkan12Features.shaderFloat16 = true;

        // Allows the use of 8 bit integers in shaders
        ctx.vulkan12Features.shaderInt8 = true;

        // Allows storage buffers to have 8bit data
        ctx.vulkan12Features.storageBuffer8BitAccess = true;

        // Allows push constants to have 8bit data
        ctx.vulkan12Features.storagePushConstant8 = true;

        // Allows the use of draw indirect count, which has the power to completely removes unneeded draw calls
        ctx.vulkan12Features.drawIndirectCount = true;

        // This is needed to create a sampler for the depth pyramid that will be used for occlusion culling
        ctx.vulkan12Features.samplerFilterMinmax = true;

        // Allows indexing into non uniform sampler arrays
        ctx.vulkan12Features.shaderSampledImageArrayNonUniformIndexing = true;

        // Allows uniform buffers to have 8bit members
        ctx.vulkan12Features.uniformAndStorageBuffer8BitAccess = true;

        ctx.vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

        // Dynamic rendering removes the need for VkRenderPass and allows the creation of rendering attachmets at draw time
        ctx.vulkan13Features.dynamicRendering = true;

        // Used for PipelineBarrier2, better sync structure API
        ctx.vulkan13Features.synchronization2 = true;

        // This is needed for local size id in shaders
        ctx.vulkan13Features.maintenance4 = true;

        VkPhysicalDeviceSynchronization2Features sync2Features{};
        sync2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
        sync2Features.synchronization2 = VK_FALSE;
        if (stats.bSynchronizationValidationSupported)
        {
            sync2Features.synchronization2 = VK_TRUE;
        }
        ctx.vulkan13Features.pNext = &sync2Features;

        ctx.vulkanFeaturesMesh.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
        if (stats.meshShaderSupport)
        {
            ctx.vulkanFeaturesMesh.meshShader = true;
            ctx.vulkanFeaturesMesh.taskShader = true;
        }
        else
        {
            ctx.vulkanFeaturesMesh.meshShader = false;
            ctx.vulkanFeaturesMesh.taskShader = false;
        }

        ctx.rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        if (stats.bRayTracingSupported)
        {
            ctx.rayQueryFeatures.rayQuery = true; 
        }
        else
        {
            ctx.rayQueryFeatures.rayQuery = false;
        }

        ctx.accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        if (stats.bRayTracingSupported)
        {
            ctx.accelerationStructureFeatures.accelerationStructure = true;
        }
        else
        {
            ctx.accelerationStructureFeatures.accelerationStructure = false;
        }

        ctx.vulkanExtendedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        ctx.vulkanExtendedFeatures.features = ctx.deviceFeatures;

        // Adds all features structs to the pNext chain
        info.pNext = &ctx.vulkanExtendedFeatures;
        ctx.vulkanExtendedFeatures.pNext = &ctx.vulkan11Features;
        ctx.vulkan11Features.pNext = &ctx.vulkan12Features;
        ctx.vulkan12Features.pNext = &ctx.vulkan13Features;
        ctx.vulkan13Features.pNext = &ctx.vulkanFeaturesMesh;
        ctx.vulkanFeaturesMesh.pNext = &ctx.rayQueryFeatures;
        ctx.rayQueryFeatures.pNext = &ctx.accelerationStructureFeatures;
    }

    static void GetVulkanQueue(VkDevice device, Queue& queue, void* pNext, VkDeviceQueueCreateFlags flags, uint32_t queueIndex = 0)
    {
        VkDeviceQueueInfo2 transferQueueInfo{};
        transferQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
        transferQueueInfo.flags = flags;
        transferQueueInfo.pNext = pNext;
        transferQueueInfo.queueFamilyIndex = queue.index;
        transferQueueInfo.queueIndex = queueIndex;

        vkGetDeviceQueue2(device, &transferQueueInfo, &queue.handle);
    }

    static uint8_t CreateDevice(VkDevice& device, VkPhysicalDevice physicalDevice, Queue& graphicsQueue, Queue& presentQueue, Queue& computeQueue, Queue& transferQueue, VulkanStats& stats)
    {
        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.flags = 0;
        deviceInfo.enabledLayerCount = 0;//Deprecated

        deviceInfo.enabledExtensionCount = stats.deviceExtensionCount;
        deviceInfo.ppEnabledExtensionNames = stats.deviceExtensionNames;

        // Features
        DeviceFeaturesContext featureContext;
        AddDeviceFeatures(deviceInfo, featureContext, stats);

        // Queue Infos to retrieve queues after device creation
        VkDeviceQueueCreateInfo queueInfos [Ce_MaxUniqueueDeviceQueueIndices] {};
        uint32_t queueCount{ 0 };

        // Graphics
        queueInfos[Ce_GraphicsQueueInfoIndex] = {};
        queueInfos[Ce_GraphicsQueueInfoIndex].queueFamilyIndex = graphicsQueue.index;
        queueCount++;

        // Dedicated transfer
        queueInfos[Ce_TransferQueueInfoIndex] = {};
        queueInfos[Ce_TransferQueueInfoIndex].queueFamilyIndex = transferQueue.index;
        queueCount++;

        // Dedicated compute (TODO: Might want to be careful here, and add it only if I am on cluster mode)
        queueInfos[Ce_ComputeQueueInfoIndex] = {};
        queueInfos[Ce_ComputeQueueInfoIndex].queueFamilyIndex = computeQueue.index;
        queueCount++;

        // If graphics has a different index from present, add a new info for it
        if (graphicsQueue.index != presentQueue.index)
        {
            queueInfos[queueCount] = {};
            queueInfos[queueCount].queueFamilyIndex = presentQueue.index;
            queueCount++;
        }

        if (queueCount > Ce_MaxUniqueueDeviceQueueIndices)
        {
            BLIT_ERROR("Something is wrong with vulkan device queue info logic");
            return 0;
        }
        
        float priority = 1.f;
        for (auto& queueInfo : queueInfos)
        {
            queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueInfo.pNext = nullptr; 
            queueInfo.flags = 0; 
            queueInfo.queueCount = 1;
            queueInfo.pQueuePriorities = &priority;
        }
        deviceInfo.queueCreateInfoCount = queueCount;
        deviceInfo.pQueueCreateInfos = queueInfos;

        // Create the device
        auto res = vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device);
        if (res != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create device");
            return 0;
        }

        // Retrieves graphics queue handle
        GetVulkanQueue(device, graphicsQueue, nullptr, 0);

        // Retrieves compute queue handle
        GetVulkanQueue(device, computeQueue, nullptr, 0);

        // Retrieves present queue handle
        GetVulkanQueue(device, presentQueue, nullptr, 0);

        // Retrieves transfer queue handle
        GetVulkanQueue(device, transferQueue, nullptr, 0);

        return 1;
    }

    static uint8_t SetupResourceManagement(VkDevice device, VkPhysicalDevice pdv, VkInstance instance, VmaAllocator& vma, MemoryCrucialHandles& memoryCrucials)
    {
        if (!CreateVmaAllocator(device, instance, pdv, vma, VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT))
        {
            BLIT_ERROR("Failed to create the vma allocator");
            return 0;
        }

        memoryCrucials.allocator = vma;
        memoryCrucials.device = device;
        memoryCrucials.instance = instance;

        // Success
        return 1;
    }

    uint8_t VulkanRenderer::Init(uint32_t windowWidth, uint32_t windowHeight, void* pPlatformHandle)
    {
        if(!CreateInstance(m_instance, &m_debugMessenger))
        {
            BLIT_ERROR("Failed to create vulkan instance");
            return 0;
        }

        if(!BlitzenPlatform::CreateVulkanSurface(m_instance, m_surface.handle, m_pCustomAllocator))
        {
            BLIT_ERROR("Failed to create Vulkan window surface");
            return 0;
        }

        if(!PickPhysicalDevice(m_physicalDevice, m_instance, m_surface.handle, m_graphicsQueue, m_computeQueue, m_presentQueue, m_transferQueue, m_stats))
        {
            BLIT_ERROR("Failed to pick suitable physical device");
            return 0;
        }

        if(!CreateDevice(m_device, m_physicalDevice, m_graphicsQueue, m_presentQueue, m_computeQueue, m_transferQueue, m_stats))
        {
            BLIT_ERROR("Failed to pick suitable physical device");
            return 0;
        }

        if(!CreateSwapchain(m_device, m_surface.handle, m_physicalDevice, windowWidth, windowHeight, m_graphicsQueue, m_presentQueue, m_computeQueue, 
            m_pCustomAllocator, m_swapchainValues))
        {
            BLIT_ERROR("Failed to create Vulkan swapchain");
            return 0;
        }

        // Commands
        for (size_t i = 0; i < ce_framesInFlight; ++i)
        {
            if (!m_frameToolsList[i].Init(m_device, m_graphicsQueue, m_transferQueue, m_computeQueue))
            {
                BLIT_ERROR("Failed to create frame tools");
                return 0;
            }
        }

        // This will be referred to by rendering attachments and will be updated when the window is resized
        m_drawExtent = {m_swapchainValues.swapchainExtent.width, m_swapchainValues.swapchainExtent.height};

        // Texture sampler. Global for all textures for now
        m_textureSampler.handle = CreateSampler(m_device, VK_FILTER_LINEAR, VK_SAMPLER_MIPMAP_MODE_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
        if (m_textureSampler.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create texture sampler");
            return 0;
        }

        m_stats.bResourceManagementReady = SetupResourceManagement(m_device, m_physicalDevice, m_instance, m_allocator, m_memoryCrucials);
        if (!m_stats.bResourceManagementReady)
        {
            BLIT_ERROR("Failed to initialize Vulkan resource management");
            return 0;
        }

        
		if (!CreateIdleDrawHandles(m_device, m_basicBackgroundPipeline.handle, m_basicBackgroundLayout.handle, m_backgroundImageSetLayout.handle, 
            m_graphicsQueue.index, m_idleCommandBufferPool.handle, m_idleDrawCommandBuffer))
		{
            BLIT_ERROR("Failed to create idle draw handles");
		    return 0;
		}

        if (!CreateLoadingTrianglePipeline(m_device, m_loadingTrianglePipeline.handle, m_loadingTriangleLayout.handle))
        {
            BLIT_ERROR("Failed to create loading triangle pipeline");
            return 0;
        }

        // Success
        return 1;
    }

    static uint8_t FindSwapchainSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info, VkFormat& swapchainFormat)
    {
        // Get the amount of available surface formats
        uint32_t surfaceFormatsCount = 0;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, nullptr) != VK_SUCCESS)
        {
            BLIT_ERROR("No surface formats (??)");
            return 0;
        }

        // Retrieves
        BlitCL::DynamicArray<VkSurfaceFormatKHR> surfaceFormats(static_cast<size_t>(surfaceFormatsCount));
        if (vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, surfaceFormats.Data()) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to retrieve surface formats");
            return 0;
        }

        // Look for the desired image format
        uint8_t found = 0;
        for (const auto& formats : surfaceFormats)
        {
            // If the desired image format is found, assigns it to the swapchain info and breaks out of the loop
            if (formats.format == VK_FORMAT_B8G8R8A8_UNORM && formats.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
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
        if (!found)
        {
            info.imageFormat = surfaceFormats[0].format;
            info.imageColorSpace = surfaceFormats[0].colorSpace;
            // Save the image format
            swapchainFormat = info.imageFormat;
        }

        return 1;
    }

    static uint8_t FindSwapchainPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info)
    {
        // Enumerate
        uint32_t presentModeCount = 0;
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to enumerate swapchain present modes (?)");
            return 0;
        }

        // Retrieve
        BlitCL::DynamicArray<VkPresentModeKHR> presentModes(static_cast<size_t>(presentModeCount));
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.Data()) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to retrieve swapchain present modes");
            return 0;
        }

        uint8_t found = 0;
        for (const auto& present : presentModes)
        {
            if (present == Ce_DesiredPresentMode)
            {
                info.presentMode = Ce_DesiredPresentMode;
                found = 1;
                BLIT_INFO("Found desired present mode");
                break;
            }
        }

        // If it was not found, sets it to this random smuck (this is supposed to be supported by everything and it's VSynced)
        if (!found)
        {
            BLIT_WARN("Desired presentation mode not found, using fallback");
            info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
        }

        return 1;
    }

    static uint8_t FindSwapchainSurfaceCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info,
        Swapchain& newSwapchain)
    {
        // Retrieves surface capabilities to properly configure some swapchain values
        VkSurfaceCapabilitiesKHR surfaceCapabilities{};
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) != VK_SUCCESS)
            return 0;

        // Updates the swapchain extent to the current extent from the surface, if it is not a special value
        if (surfaceCapabilities.currentExtent.width != UINT32_MAX)
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
        if (surfaceCapabilities.maxImageCount > 0 && surfaceCapabilities.maxImageCount < imageCount)
        {
            imageCount = surfaceCapabilities.maxImageCount;
        }

        // Swapchain image count fully checked and ready to be pass to the swapchain info
        info.minImageCount = imageCount;

        info.preTransform = surfaceCapabilities.currentTransform;

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
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainInfo.surface = surface;
        
        swapchainInfo.imageUsage = Ce_SwapchainImageUsageFlags;
        
        swapchainInfo.oldSwapchain = oldSwapchain;

        // Finds the surface format, updates the swapchain info and swapchain struct if it succeeds
        if (!FindSwapchainSurfaceFormat(physicalDevice, surface, swapchainInfo, newSwapchain.swapchainFormat))
        {
            BLIT_ERROR("Failed to find swapchain surface format");
            return 0;
        }

        // Finds the present mode, updates the swapchain info if it succeeds
        if (!FindSwapchainPresentMode(physicalDevice, surface, swapchainInfo))
        {
            BLIT_ERROR("Failed to find swapchain presentation mode");
            return 0;
        }

        // Sets the swapchain extent to the window's width and height
        newSwapchain.swapchainExtent = {windowWidth, windowHeight};

        // Compare the current swapchain stats to the surface capabilities
        if (!FindSwapchainSurfaceCapabilities(physicalDevice, surface, swapchainInfo, newSwapchain))
        {
            BLIT_ERROR("Failed to find swapchain surface surface capabilities");
            return 0;
        }

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
        VkResult swapchainResult = vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &newSwapchain.swapchainHandle);
        if (swapchainResult != VK_SUCCESS)
        {
            BLIT_ERROR("Failed at swapchain creation");
            return 0;
        }

        // Retrieve the swapchain image count
        uint32_t swapchainImageCount = 0;
        VkResult imageResult = vkGetSwapchainImagesKHR(device, newSwapchain.swapchainHandle, &swapchainImageCount, nullptr);
        if (imageResult != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to enumerate swapchain images");
            return 0;
        }

        // Resize the swapchain images array and pass the swapchain images
        newSwapchain.swapchainImages.Resize(swapchainImageCount);
        imageResult = vkGetSwapchainImagesKHR(device, newSwapchain.swapchainHandle, &swapchainImageCount, newSwapchain.swapchainImages.Data());
        if (imageResult != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to retrieve swapchain images");
            return 0;
        }

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
            if (!CreateImageView(device, view, newSwapchain.swapchainImages[i], newSwapchain.swapchainFormat, 0, 1))
            {
                BLIT_ERROR("Failed to create swapchain image views");
                return 0;
            }
        }
        
        // If the swapchain has been created and the images have been acquired, swapchain creation is succesful
        return 1;
    }

    uint8_t CreateIdleDrawHandles(VkDevice device, VkPipeline& pipeline, VkPipelineLayout& layout, VkDescriptorSetLayout& setLayout, 
        uint32_t queueIndex, VkCommandPool& commandPool, VkCommandBuffer& commandBuffer)
    {
        VkDescriptorSetLayoutBinding backgroundImageLayoutBinding{};
        CreateDescriptorSetLayoutBinding(backgroundImageLayoutBinding, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);

        setLayout = CreateDescriptorSetLayout(device, 1, &backgroundImageLayoutBinding, VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
        if (setLayout == VK_NULL_HANDLE)
        {
            return 0;
        }

        // Creates the layout for the background compute shader
        VkPushConstantRange backgroundImageShaderPushConstant{};
        CreatePushConstantRange(backgroundImageShaderPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(BackgroundShaderPushConstant));
        if (!CreatePipelineLayout(device, &layout, Ce_SinglePointer, &setLayout, Ce_SinglePointer, &backgroundImageShaderPushConstant))
        {
            BLIT_ERROR("Failed to create background image pipeline layout");
            return 0;
        }

        // Create the background shader in case the renderer has not objects
        if (!CreateComputeShaderProgram(device, "VulkanShaders/BasicBackground.comp.glsl.spv",
            VK_SHADER_STAGE_COMPUTE_BIT, "main", layout, &pipeline))
        {
            BLIT_ERROR("Failed to create BasicBackground.comp shader program");
            return 0;
        }

        VkCommandPoolCreateInfo idleCommandPoolInfo{};
        CreateCommandPoolInfo(idleCommandPoolInfo, queueIndex, nullptr);
        if (vkCreateCommandPool(device, &idleCommandPoolInfo, nullptr, &commandPool) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create idle draw command buffer pool");
            return 0;
        }

        VkCommandBufferAllocateInfo idleCommandBufferInfo{};
        CreateCmdbInfo(idleCommandBufferInfo, commandPool);
        if (vkAllocateCommandBuffers(device, &idleCommandBufferInfo, &commandBuffer) != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to allocate idle draw command buffer");
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