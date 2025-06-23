#include "vulkanInit.h"
#include "Renderer/BlitzenVulkan/Context/vulkanRenderer.h"
#include "Renderer/BlitzenVulkan/RuntimeHelpers/vulkanCommands.h"
#include "Platform/blitPlatform.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanResourceFunctions.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanPipelines.h"
#include "Renderer/BlitzenVulkan/Resources/vulkanRNDResources.h"
#include "BlitCL/blitDynamicArr.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include <cstring> // For strcmp

namespace BlitzenVulkan
{
    static void CreateApplicationInfo(VkApplicationInfo& appInfo, void* pNext, const char* appName, uint32_t appVersion, const char* engineName, uint32_t engineVersion, 
        uint32_t apiVersion = VK_API_VERSION_1_3)
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

    static uint8_t FindInstanceExtensions(VkInstanceCreateInfo& instanceInfo, VulkanStats& ctx, BlitCL::DynamicArray<VkExtensionProperties>& availableExtensions)
    {
        uint32_t extensionCount = 0;

        static uint8_t STRCMP_TRUE = 0;

        uint8_t found = 0;
        for (auto& extension : availableExtensions)
        {
            if (strcmp(extension.extensionName, ce_surfaceExtensionName) == STRCMP_TRUE)
            {
                ctx.m_instExtensions.PushBack(ce_surfaceExtensionName);
                found = 1;
                break;
            }
        }

        if (!found)
        {
            BLIT_ERROR("Failed to find platform specific surface extension");
            return 0;
        }

        found = 0;
        for (auto& extension : availableExtensions)
        {
            if (strcmp(extension.extensionName, "VK_KHR_surface") == STRCMP_TRUE)
            {
                ctx.m_instExtensions.PushBack("VK_KHR_surface");
                found = 1;
                break;
            }
        }

        if (!found)
        {
            BLIT_ERROR("Failed to find KHR surface extension");
            return 0;
        }

        
        if constexpr (ce_bValidationLayersRequested)
        {
            found = 0;
            for (auto& extension : availableExtensions)
            {
                if (strcmp(extension.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == STRCMP_TRUE)
                {
                    ctx.m_instExtensions.PushBack(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                BLIT_ERROR("Failed to load validation layers extension. Vulkan will not be used without validation layers in debug mode");
                return 0;
            }
        }

        instanceInfo.ppEnabledExtensionNames = ctx.m_instExtensions.Data();
        instanceInfo.enabledExtensionCount = (uint32_t)ctx.m_instExtensions.GetSize();

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

    // Debug messenger callback function
    static VKAPI_ATTR VkBool32 VKAPI_CALL S_DEBUG_CALLBACK(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
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
        debugMessengerInfo.pfnUserCallback = S_DEBUG_CALLBACK;

        debugMessengerInfo.pNext = nullptr;
        debugMessengerInfo.pUserData = nullptr;

        return 1;
    }

    static uint8_t EnabledInstanceSynchronizationValidation()
    {
        // Getting all supported validation layers
        uint32_t availableLayerCount = 0;
        vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
        BlitCL::DynamicArray<VkLayerProperties> availableLayers{ size_t(availableLayerCount) };
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
            if constexpr (Ce_SyncValidationRequested)
            {
                if (EnabledInstanceSynchronizationValidation())
                {
                    instanceInfo.enabledLayerCount = 2;
                }
                else
                {
                    BLIT_WARN("Failed to enable Vulkan synchronization validation");
                    instanceInfo.enabledLayerCount = 1;
                }
            }
            else
            {
                instanceInfo.enabledLayerCount = 1;
            }

            instanceInfo.ppEnabledLayerNames = ppEnabledLayerNames;

            return 1;
        }

        BLIT_ERROR("Failed to enable validation layers");
        return 0;
    }

    uint8_t CreateInstance(VkInstance& instance, VulkanStats& stats, VkDebugUtilsMessengerEXT* pDM /*= nullptr*/)
    {
        uint32_t apiVersion = 0;
        VkResult versionRes{ vkEnumerateInstanceVersion(&apiVersion) };
        if (versionRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to enumerate instance version");
            return VK_LOG_ERROR_MSG_AND_RETURN(versionRes);
        }

        if (apiVersion < Ce_VkApiVersion)
        {
            BLIT_ERROR("Required Vulkan API version not supported");
            return 0;
        }

        const char* appName{ BlitzenCore::Ce_HostedApp };
        const char* engineName{ BlitzenCore::CE_BLITZEN };
        VkApplicationInfo applicationInfo{};
        CreateApplicationInfo(applicationInfo, nullptr, appName, VK_MAKE_VERSION(BlitzenCore::Ce_HostedAppVersion, 0, 0), engineName, VK_MAKE_VERSION(BlitzenCore::Ce_BlitzenMajor, 0, 0));

        VkInstanceCreateInfo instanceInfo{};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.pNext = nullptr;
        instanceInfo.flags = 0;
        instanceInfo.pApplicationInfo = &applicationInfo;

        // Enumerates
        uint32_t availableExtensionCount = 0;
        VkResult extensionCountRes{ vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr) };
        if (extensionCountRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to enumerate instance extensions");
            return VK_LOG_ERROR_MSG_AND_RETURN(extensionCountRes);
        }

        if (availableExtensionCount == 0)
        {
            BLIT_ERROR("instance extension count returned zero. This should not happen!");
            return 0;
        }

        BlitCL::DynamicArray<VkExtensionProperties> availableExtensions{ availableExtensionCount };
        VkResult extensionQueryRes{ vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.Data()) };
        if (extensionQueryRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to query instance extensions");
            return VK_LOG_ERROR_MSG_AND_RETURN(extensionQueryRes);
        }

        // Finds extensions
        if (!FindInstanceExtensions(instanceInfo, stats, availableExtensions))
        {
            BLIT_ERROR("Failed to find all required instance extensions");
            return 0;
        }

        VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo{};

        // Shader printf
        VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT };
        VkValidationFeaturesEXT validationFeatures{};

        const char* layerNames[2] = { ce_baseValidationLayerName, Ce_SyncValidationLayerName };

        instanceInfo.enabledLayerCount = 0;

        if constexpr (ce_bValidationLayersRequested)
        {
            if (!EnableValidationLayers(instanceInfo, debugMessengerInfo, validationFeatures, enables, layerNames))
            {
                BLIT_ERROR("Failed to enable validation layers");
                return 0;
            }
        }

        VkResult instanceRes = vkCreateInstance(&instanceInfo, nullptr, &instance);
        if (instanceRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create instance");
            return VK_LOG_ERROR_MSG_AND_RETURN(instanceRes);
        }

        if constexpr (ce_bValidationLayersRequested)
        {
            VkResult debugMsgRes = CreateDebugUtilsMessengerEXT(instance, &debugMessengerInfo, nullptr, pDM);
            if (debugMsgRes != VK_SUCCESS)
            {
                BLIT_ERROR("Failed to create debug messenger for validation layers");
                return VK_LOG_ERROR_MSG_AND_RETURN(debugMsgRes);
            }
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

        vkGetPhysicalDeviceFeatures2(pdv, &features2);

        // Check that all the required features are supported by the device
        if (!features.multiDrawIndirect || !features.samplerAnisotropy ||
            !features11.storageBuffer16BitAccess || !features11.shaderDrawParameters ||
            !features12.bufferDeviceAddress || !features12.descriptorIndexing || !features12.runtimeDescriptorArray || !features12.storageBuffer8BitAccess ||
            !features12.shaderFloat16 || !features12.drawIndirectCount || !features12.samplerFilterMinmax || !features12.shaderInt8 || 
            !features12.shaderSampledImageArrayNonUniformIndexing ||!features12.uniformAndStorageBuffer8BitAccess || !features12.storagePushConstant8 ||
            !features13.synchronization2 || !features13.dynamicRendering || !features13.maintenance4)
        {
            BLIT_ERROR("Failed to find required device feature");
            return 0;
        }

        if constexpr (Ce_MeshShadersRequested)
        {
            VkPhysicalDeviceMeshShaderFeaturesEXT meshFeatures{};
            meshFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
            features13.pNext = &meshFeatures;
            vkGetPhysicalDeviceFeatures2(pdv, &features2);

            if (!meshFeatures.meshShader || !meshFeatures.taskShader)
            {
                BLIT_ERROR("Failed to find mesh shader feature support. This feature can be deactivate");
                return 0;
            }
        }

        if constexpr (Ce_RayTracingRequested)
        {
            VkPhysicalDeviceRayQueryFeaturesKHR rayQuery{};
            rayQuery.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
            features2.pNext = &rayQuery;
            VkPhysicalDeviceAccelerationStructureFeaturesKHR ASfeats{};
            ASfeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            rayQuery.pNext = &ASfeats;
            vkGetPhysicalDeviceFeatures2(pdv, &features2);

            if (!rayQuery.rayQuery || !ASfeats.accelerationStructure)
            {
                BLIT_ERROR("Failed to find required feature support for ray tracing. RT can be deactivated");
                return 0;
            }
        }

        return 1;
    }

    static uint8_t LookForRequestedExtensions(VkPhysicalDevice pdv, VulkanStats& stats)
    {
        const uint8_t STRCMP_TRUE = 0;

        // Checking if the device supports all extensions that will be requested from Vulkan
        uint32_t dvExtensionCount = 0;
        VkResult dvExtensionCountRes{ vkEnumerateDeviceExtensionProperties(pdv, nullptr, &dvExtensionCount, nullptr) };
        if (dvExtensionCountRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to enumerate device extensions");
            return VK_LOG_ERROR_MSG_AND_RETURN(dvExtensionCountRes);
        }

        if (dvExtensionCount == 0)
        {
            BLIT_ERROR("Device extension query returned count zero. This should not happen!");
            return 0;
        }

        BlitCL::DynamicArray<VkExtensionProperties> dvExtensionsProps{ size_t(dvExtensionCount) };
        VkResult dvExtensionsQueryRes{ vkEnumerateDeviceExtensionProperties(pdv, nullptr, &dvExtensionCount, dvExtensionsProps.Data()) };
        if (dvExtensionsQueryRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to get device extensions");
            return VK_LOG_ERROR_MSG_AND_RETURN(dvExtensionsQueryRes);
        }

        // SWAPCHAIN
        uint8_t found = 0;
        for (auto& extension : dvExtensionsProps)
        {
            if (strcmp(extension.extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME) == STRCMP_TRUE)
            {
                stats.m_dvExtensions.PushBack(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
                found = 1;
                break;
            }
        }

        if (!found)
        {
            BLIT_ERROR("Could not find support for swapchain extension");
            return 0;
        }

        // PUSH DESCRIPTORS
        found = 0 ;
        for (auto& extension : dvExtensionsProps)
        {
            if (strcmp(extension.extensionName, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME) == STRCMP_TRUE)
            {
                stats.m_dvExtensions.PushBack(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
                found = 1;
                break;
            }
        }

        if (!found)
        {
            BLIT_ERROR("Cound not find support for push descriptor extension. Push descriptors are required for Blitzen Vulkan");
            return 0;
        }

        if constexpr (Ce_GPUPrintfRequested)
        {
            found = 0;
            for (auto& extension : dvExtensionsProps)
            {
                if (strcmp(extension.extensionName, VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME) == STRCMP_TRUE)
                {
                    stats.m_dvExtensions.PushBack(VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME);
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                BLIT_ERROR("Could not find support for debug printf shader extension. This extension can be deactivated");
                return 0;
            }
        }

        if constexpr (Ce_MeshShadersRequested)
        {
            found = 0;
            for (auto& extension : dvExtensionsProps)
            {
                if (strcmp(extension.extensionName, VK_EXT_MESH_SHADER_EXTENSION_NAME) == STRCMP_TRUE)
                {
                    stats.m_dvExtensions.PushBack(VK_EXT_MESH_SHADER_EXTENSION_NAME);
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                BLIT_ERROR("Could not find support for mesh shader EXT extension.");
                BLIT_INFO("Mesh shaders can be deactivated");
                return 0;
            }
        }

        if constexpr (Ce_RayTracingRequested)
        {
            found = 0;
            for (auto& extension : dvExtensionsProps)
            {
                if (strcmp(extension.extensionName, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == STRCMP_TRUE)
                {
                    stats.m_dvExtensions.PushBack(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                BLIT_ERROR("Could not find support for acceleration structure extension, which is required for raytracing");
                BLIT_INFO("Ray tracing can be deactivated");
                return 0;
            }

            found = 0;
            for (auto& extension : dvExtensionsProps)
            {
                if (strcmp(extension.extensionName, VK_KHR_RAY_QUERY_EXTENSION_NAME) == STRCMP_TRUE)
                {
                    stats.m_dvExtensions.PushBack(VK_KHR_RAY_QUERY_EXTENSION_NAME);
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                BLIT_ERROR("Could not find support for ray query extension, which is required for raytracing");
                BLIT_INFO("Ray tracing can be deactivated");
                return 0;
            }

            found = 0;
            for (auto& extension : dvExtensionsProps)
            {
                if (strcmp(extension.extensionName, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME) == STRCMP_TRUE)
                {
                    stats.m_dvExtensions.PushBack(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                BLIT_ERROR("Could not find support for deferred host operations extension, which is required for raytracing");
                BLIT_INFO("Ray tracing can be deactivated");
                return 0;
            }
        }

        if constexpr (Ce_DynamicRenderingExtensionRequested)
        {
            found = 0;
            for (auto& extension : dvExtensionsProps)
            {
                if (strcmp(extension.extensionName, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME) == STRCMP_TRUE)
                {
                    stats.m_dvExtensions.PushBack(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                BLIT_ERROR("Could not find support for dynamic rendering, which is required for IMGUI editor");
                return 0;
            }
        }

        return 1;
    }

    static uint8_t ValidatePdvQueueFamilies(VkPhysicalDevice pdv, VkSurfaceKHR surface, Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, Queue& transferQueue, VulkanStats& stats, 
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

        if (BlitzenCore::Ce_BuildClusters && !computeQueue.hasIndex)
        {
            BLIT_ERROR("Vulkan Cluster mode needs dedicated compute queue");
            return 0;
        }

        return 1;
    }

    uint8_t ValidatePhysicalDevice(VkPhysicalDevice pdv, VkInstance instance, VkSurfaceKHR surface, Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, Queue& transferQueue, VulkanStats& stats)
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
        if (queueFamilyPropertyCount == 0)
        {
            BLIT_ERROR("Physical device queue family properties returned count 0. This should not happen");
            return 0;
        }


        BlitCL::DynamicArray<VkQueueFamilyProperties2> queueFamilyProperties{ size_t(queueFamilyPropertyCount), {} };
        for (auto& queueProps : queueFamilyProperties)
        {
            queueProps.sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
        }

        vkGetPhysicalDeviceQueueFamilyProperties2(pdv, &queueFamilyPropertyCount, queueFamilyProperties.Data());

        // Queue families
        if(!ValidatePdvQueueFamilies(pdv, surface, graphicsQueue, computeQueue, presentQueue, transferQueue, stats, queueFamilyProperties))
        {
            BLIT_ERROR("Failed to validate physical device queue family properties");
            return 0;
        }

        return 1;

    }

    uint8_t PickPhysicalDevice(VkPhysicalDevice& gpu, VkInstance instance, VkSurfaceKHR surface, Queue& graphicsQueue, Queue& computeQueue, Queue& presentQueue, Queue& transferQueue,  VulkanStats& stats)
    {
        // Retrieves the physical device count
        uint32_t physicalDeviceCount = 0;
        VkResult pdvCountRes{ vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr) };
        if (pdvCountRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to enumerate physical devices")
            return VK_LOG_ERROR_MSG_AND_RETURN(pdvCountRes);
        }

        if (physicalDeviceCount == 0)
        {
            BLIT_ERROR("Physical device count returned 0. This should not happen");
            return 0;
        }

        // Pass the available devices to an array to pick the best one
        BlitCL::DynamicArray<VkPhysicalDevice> physicalDevices{ physicalDeviceCount };
        VkResult pdvQueryRes{ vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.Data()) };
        if (pdvQueryRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to query for available physical devices");
            return VK_LOG_ERROR_MSG_AND_RETURN(pdvQueryRes);
        }

        if (physicalDeviceCount != physicalDevices.GetSize())
        {
            BLIT_ERROR("Physical device count query inconsistency");
            return 0;
        }

        uint32_t extensionCount = 0;
        for (auto& pdv : physicalDevices)
        {
            VkPhysicalDeviceProperties props{};
            vkGetPhysicalDeviceProperties(pdv, &props);
            
            if (props.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
            {
                BLIT_INFO("Skipping no discrete GPU");
                continue;
            }
            
            if (ValidatePhysicalDevice(pdv, instance, surface, graphicsQueue, computeQueue, presentQueue, transferQueue, stats))
            {
                gpu = pdv;
                stats.hasDiscreteGPU = 1;
                BLIT_INFO("Discrete GPU found");
                return 1;
            }
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
        }

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

        ctx.vulkanFeaturesMesh.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
        ctx.vulkanFeaturesMesh.meshShader = false;
        ctx.vulkanFeaturesMesh.taskShader = false;
        if (stats.meshShaderSupport)
        {
            ctx.vulkanFeaturesMesh.meshShader = true;
            ctx.vulkanFeaturesMesh.taskShader = true;
        }

        ctx.rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
        ctx.rayQueryFeatures.rayQuery = false;
        if (stats.bRayTracingSupported)
        {
            ctx.rayQueryFeatures.rayQuery = true; 
        }

        ctx.accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
        ctx.accelerationStructureFeatures.accelerationStructure = false;
        if (stats.bRayTracingSupported)
        {
            ctx.accelerationStructureFeatures.accelerationStructure = true;
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

    void GetVulkanQueue(VkDevice device, Queue& queue, void* pNext, VkDeviceQueueCreateFlags flags, uint32_t queueIndex /*=0*/)
    {
        VkDeviceQueueInfo2 transferQueueInfo{};
        transferQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
        transferQueueInfo.flags = flags;
        transferQueueInfo.pNext = pNext;
        transferQueueInfo.queueFamilyIndex = queue.index;
        transferQueueInfo.queueIndex = queueIndex;

        vkGetDeviceQueue2(device, &transferQueueInfo, &queue.handle);
    }

    uint8_t CreateDevice(VkDevice& device, VkPhysicalDevice physicalDevice, Queue& graphicsQueue, Queue& presentQueue, Queue& computeQueue, Queue& transferQueue, VulkanStats& stats)
    {
        VkDeviceCreateInfo deviceInfo{};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.flags = 0;
        deviceInfo.enabledLayerCount = 0;//Deprecated

        deviceInfo.enabledExtensionCount = (uint32_t)stats.m_dvExtensions.GetSize();
        deviceInfo.ppEnabledExtensionNames = stats.m_dvExtensions.Data();

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
        VkResult deviceCreationRes{ vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &device) };
        if (deviceCreationRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to create device");
            return VK_LOG_ERROR_MSG_AND_RETURN(deviceCreationRes);
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

    uint8_t SetupResourceManagement(VkDevice device, VkPhysicalDevice pdv, VkInstance instance, VmaAllocator& vma, MemoryCrucialHandles& memoryCrucials)
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

    static uint8_t FindSwapchainSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info, VkFormat& swapchainFormat)
    {
        // Get the amount of available surface formats
        uint32_t surfaceFormatsCount = 0;
        VkResult surfaceFormatCountResult{ vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, nullptr) };
        if (surfaceFormatCountResult != VK_SUCCESS)
        {
            BLIT_ERROR("Get physical device surface formats returned 0. This should not happen, and there might be something wrong with general initialization logic");
            return VK_LOG_ERROR_MSG_AND_RETURN(surfaceFormatCountResult);
        }

        // Retrieves
        BlitCL::DynamicArray<VkSurfaceFormatKHR> surfaceFormats(static_cast<size_t>(surfaceFormatsCount));
        VkResult surfaceQueryResult{ vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, surfaceFormats.Data()) };
        if (surfaceQueryResult != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to retrieve surface formats from physical device");
            return VK_LOG_ERROR_MSG_AND_RETURN(surfaceQueryResult);
        }

        for (const auto& formats : surfaceFormats)
        {
            // If the desired image format is found, assigns it to the swapchain info and breaks out of the loop
            if (formats.format == Ce_DesiredSwapchainSurfaceFormat && formats.colorSpace == Ce_DesiredSwapchainColorSpace)
            {
                info.imageFormat = Ce_DesiredSwapchainSurfaceFormat;
                info.imageColorSpace = Ce_DesiredSwapchainColorSpace;
                // Saves the format to init handles
                swapchainFormat = Ce_DesiredSwapchainSurfaceFormat;
                return 1;
            }
        }

        // Checks other available formats
        for (const auto& format : surfaceFormats)
        {
            VkFormatProperties formatProps;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, format.format, &formatProps);

            // Color attachment support
            if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
            {
                info.imageFormat = format.format;
                info.imageColorSpace = format.colorSpace;
                swapchainFormat = format.format;
                return 1;
            }
        }
       
        BLIT_ERROR("Failed to find suitable swapchain surface formats");
        return 0;
    }

    static uint8_t FindSwapchainPresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info)
    {
        // Enumerate
        uint32_t presentModeCount = 0;
        VkResult presentModeCountResult{ vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr) };
        if (presentModeCountResult != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to enumerate swapchain present modes. This should not happen and indicates a problem with initialization logic");
            return VK_LOG_ERROR_MSG_AND_RETURN(presentModeCountResult);
        }

        // Retrieve
        BlitCL::DynamicArray<VkPresentModeKHR> presentModes{ size_t(presentModeCount) };
        VkResult presentModeQueryResult{ vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.Data()) };
        if (presentModeQueryResult != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to retrieve swapchain present modes");
            return VK_LOG_ERROR_MSG_AND_RETURN(presentModeQueryResult);
        }

        for (const auto& present : presentModes)
        {
            if (present == Ce_DesiredPresentMode)
            {
                info.presentMode = Ce_DesiredPresentMode;
                BLIT_INFO("Found desired present mode");
                return 1;
            }
        }

        // If desired mode is not found, check if FIFO is supported
        for (const auto& present : presentModes)
        {
            if (present == VK_PRESENT_MODE_FIFO_KHR)
            {
                info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
                BLIT_WARN("Desired present mode not found, using FIFO mode as fallback.");
                return 1;
            }
        }

        BLIT_ERROR("Failed to find present mode for swapchain. This should not happen! There might be something wrong with vulkan init logic");
        return 0;
    }

    static uint8_t FindSwapchainSurfaceCapabilities(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkSwapchainCreateInfoKHR& info, Swapchain& newSwapchain)
    {
        VkSurfaceCapabilitiesKHR surfaceCapabilities{};
        VkResult surfaceCapabilitiesRes{ vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities) };
        if (surfaceCapabilitiesRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to retrieve surface capabilities");
            return VK_LOG_ERROR_MSG_AND_RETURN(surfaceCapabilitiesRes);
        }

        if (surfaceCapabilities.currentExtent.width != UINT32_MAX)
        {
            newSwapchain.m_extent = surfaceCapabilities.currentExtent;
        }

        // Gets the min extent and max extent allowed by the GPU,  to clamp the initial value
        VkExtent2D minExtent = surfaceCapabilities.minImageExtent;
        VkExtent2D maxExtent = surfaceCapabilities.maxImageExtent;

        newSwapchain.m_extent.width = BlitML::Clamp(newSwapchain.m_extent.width, maxExtent.width, minExtent.width);
        newSwapchain.m_extent.height = BlitML::Clamp(newSwapchain.m_extent.height, maxExtent.height, minExtent.height);

        info.imageExtent = newSwapchain.m_extent;

        uint32_t minImageCount = surfaceCapabilities.minImageCount;
        if (surfaceCapabilities.maxImageCount > 0 && surfaceCapabilities.maxImageCount < minImageCount + 1)
        {
            minImageCount = surfaceCapabilities.maxImageCount;
        }

        if (minImageCount < ce_framesInFlight)
        {
            BLIT_ERROR("Swapchain does not support buffer count: %u", ce_framesInFlight);
            return 0;
        }

        info.minImageCount = minImageCount;
        newSwapchain.m_minImageCount = minImageCount;

        info.preTransform = surfaceCapabilities.currentTransform;

        return 1;
    }

    uint8_t CreateSwapchain(VkDevice device, VkSurfaceKHR surface, VkPhysicalDevice physicalDevice, uint32_t windowWidth, uint32_t windowHeight, 
        Queue graphicsQueue, Queue presentQueue, Queue computeQueue, VkAllocationCallbacks* pCustomAllocator, Swapchain& newSwapchain, VkSwapchainKHR oldSwapchain)
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
        if (!FindSwapchainSurfaceFormat(physicalDevice, surface, swapchainInfo, newSwapchain.m_format))
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
        newSwapchain.m_extent = {windowWidth, windowHeight};

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
            swapchainInfo.queueFamilyIndexCount = 0;
            swapchainInfo.pQueueFamilyIndices = queueFamilyIndices;
        }

        // Create the swapchain
        VkResult swapchainResult = vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &newSwapchain.m_handle);
        if (swapchainResult != VK_SUCCESS)
        {
            BLIT_ERROR("Failed at swapchain creation");
            return VK_LOG_ERROR_MSG_AND_RETURN(swapchainResult);
        }

        // Retrieve the swapchain image count
        uint32_t swapchainImageCount = 0;
        VkResult imageCountRes = vkGetSwapchainImagesKHR(device, newSwapchain.m_handle, &swapchainImageCount, nullptr);
        if (imageCountRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to enumerate swapchain images");
            return VK_LOG_ERROR_MSG_AND_RETURN(imageCountRes);
        }

        if (swapchainImageCount > Ce_MaxSwapchainImageCount)
        {
            BLIT_ERROR("Swapchain Image count: %u bigger than max image count: %u", swapchainImageCount, Ce_MaxSwapchainImageCount);
            return 0;
        }

        newSwapchain.m_imageCount = swapchainImageCount;
        VkResult imageQueryRes = vkGetSwapchainImagesKHR(device, newSwapchain.m_handle, &swapchainImageCount, newSwapchain.m_images);
        if (imageQueryRes != VK_SUCCESS)
        {
            BLIT_ERROR("Failed to retrieve swapchain images");
            return VK_LOG_ERROR_MSG_AND_RETURN(imageCountRes);
        }

        if (newSwapchain.m_imageCount != swapchainImageCount)
        {
            BLIT_ERROR("Inconsistency with swapchain image count result");
            return 0;
        }

        for (uint32_t image = 0; image < newSwapchain.m_imageCount; image++)
        {
            if (!CreateImageView(device, newSwapchain.m_views[image], newSwapchain.m_images[image], newSwapchain.m_format, 0, 1))
            {
                BLIT_ERROR("Failed to create swapchain image view");
                return 0;
            }
        }
        
        // success
        return 1;
    }

    static VkDescriptorSetLayout CreateGPUBufferPushDescriptorBindings(VkDevice device, BlitCL::DynamicArray<VkDescriptorSetLayoutBinding>& bindings, uint8_t bRaytracing, uint8_t bMeshShaders)
    {
        auto viewDataShaderStageFlags = bMeshShaders ? VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT :
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_COMPUTE_BIT;

        uint32_t bindingCount = 0;
        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_ViewDataBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, viewDataShaderStageFlags);

        auto vertexBufferShaderStageFlags = bMeshShaders ? VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT : VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_VertexBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, vertexBufferShaderStageFlags);

        auto surfaceBufferShaderStageFlags = bMeshShaders ? VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT :
            VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_SurfaceBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, surfaceBufferShaderStageFlags);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_HI_Z_CullBinding, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_LODBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_TransformBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_MatBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_DrawCmdBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_RenderBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_DrawCmdCounterDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_DrawVisBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

        CreateDescriptorSetLayoutBinding(bindings[bindingCount++], Ce_BoundingSphereDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT);

        if (BlitzenCore::Ce_BuildClusters)
        {
            auto clusterBufferShaderStageFlags{ bMeshShaders ? VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_MESH_BIT_EXT | VK_SHADER_STAGE_TASK_BIT_EXT : VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_VERTEX_BIT };
            VkDescriptorSetLayoutBinding clusterBinding{};
            CreateDescriptorSetLayoutBinding(clusterBinding, Ce_ClusterBufferDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, clusterBufferShaderStageFlags);

            bindings.PushBack(clusterBinding);
        }

        if (bRaytracing)
        {
            VkDescriptorSetLayoutBinding tlasBinding{};
            CreateDescriptorSetLayoutBinding(tlasBinding, Ce_TlasBufferBinding, 1, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, VK_SHADER_STAGE_FRAGMENT_BIT);

            bindings.PushBack(tlasBinding);
        }

        return CreateDescriptorSetLayout(device, (uint32_t)bindings.GetSize(), bindings.Data(), VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
    }

    uint8_t CreateDescriptorLayouts(VkDevice device, DescriptorContext& descriptorContext, VulkanStats& stats, uint32_t textureCount)
    {
        // The big GPU push descriptor set layout. Holds most buffers
        BlitCL::DynamicArray<VkDescriptorSetLayoutBinding> pushDescriptorBindings{ Ce_DefaultPushDescriptorBindingCount, {} };

        descriptorContext.m_pushDescriptorLayout.handle = CreateGPUBufferPushDescriptorBindings(device, pushDescriptorBindings, stats.bRayTracingSupported, stats.meshShaderSupport);
        if (descriptorContext.m_pushDescriptorLayout.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create GPU buffer push descriptor layout");
            return 0;
        }

        // Descriptor set layout for textures
        VkDescriptorSetLayoutBinding texturesLayoutBinding{};
        CreateDescriptorSetLayoutBinding(texturesLayoutBinding, Ce_TextureDescriptorsBinding, BlitzenCore::Ce_MaxTextureCount, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
        descriptorContext.m_textureDescriptorSetlayout.handle = CreateDescriptorSetLayout(device, 1, &texturesLayoutBinding);
        if (descriptorContext.m_textureDescriptorSetlayout.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create texture descriptor set layout");
            return 0;
        }

        VkDescriptorSetLayoutBinding depthPyramidBindings[Ce_HI_Z_DescriptorCount]{};
        CreateDescriptorSetLayoutBinding(depthPyramidBindings[0], Ce_HI_Z_DstImageBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
        CreateDescriptorSetLayoutBinding(depthPyramidBindings[1], Ce_HI_Z_SrcImageBinding, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);
        descriptorContext.m_HI_Z_descriptorSetLayout.handle = CreateDescriptorSetLayout(device, Ce_HI_Z_DescriptorCount, depthPyramidBindings, VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
        if (descriptorContext.m_HI_Z_descriptorSetLayout.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create depth pyramid descriptor set layout");
            return 0;
        }

        VkDescriptorSetLayoutBinding presentGenerationBindings[2]{};
        CreateDescriptorSetLayoutBinding(presentGenerationBindings[0], Ce_SwapchainDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
        CreateDescriptorSetLayoutBinding(presentGenerationBindings[1], Ce_ColorTargetDescriptorBinding, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT);

        descriptorContext.m_presentSetlayout.handle = CreateDescriptorSetLayout(device, 2, presentGenerationBindings, VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR);
        if (descriptorContext.m_presentSetlayout.handle == VK_NULL_HANDLE)
        {
            BLIT_ERROR("Failed to create present image generation layout");
            return 0;
        }

        // Success 
        return 1;
    }

    uint8_t CreatePipelineLayouts(VkDevice device, PipelineContext& context, DescriptorContext& descriptorContext)
    {
        // OPAQUE GRAPHICS
        VkDescriptorSetLayout opaqueDrawDescLayouts[2] = { descriptorContext.m_pushDescriptorLayout.handle, descriptorContext.m_textureDescriptorSetlayout.handle };

        if (!CreatePipelineLayout(device, &context.m_opaqueDrawLayout.handle, BLIT_ARRAY_SIZE(opaqueDrawDescLayouts), opaqueDrawDescLayouts, 0, nullptr))
        {
            BLIT_ERROR("Failed to create main graphics pipeline layout");
            return 0;
        }

        // CULLING SHADERS
        VkPushConstantRange cullShaderPushConstant{};
        CreatePushConstantRange(cullShaderPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(uint32_t));

        if (!CreatePipelineLayout(device, &context.m_drawCullLayout.handle, 1, &descriptorContext.m_pushDescriptorLayout.handle, 1, &cullShaderPushConstant))
        {
            BLIT_ERROR("Failed to create culling pipeline layout");
            return 0;
        }

        // CLUSTER CULLING SHADERS
        if (BlitzenCore::Ce_BuildClusters)
        {
            VkPushConstantRange clusterCullPushConstant{};
            CreatePushConstantRange(clusterCullPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(uint32_t));
            if (!CreatePipelineLayout(device, &context.m_clusterCullLayout.handle, 1, &descriptorContext.m_pushDescriptorLayout.handle, 1, &clusterCullPushConstant))
            {
                BLIT_ERROR("Failed to create culling pipeline layout");
                return 0;
            }
        }

        // HI Z SHADER
        VkPushConstantRange HI_Z_pushConstant{};
        CreatePushConstantRange(HI_Z_pushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(BlitML::vec2));

        if (!CreatePipelineLayout(device, &context.m_hiZLayout.handle, 1, &descriptorContext.m_HI_Z_descriptorSetLayout.handle, 1, &HI_Z_pushConstant))
        {
            BLIT_ERROR("Failed to create depth pyramid generation pipeline layout");
            return 0;
        }

        // Generate present image compute shader layout
        VkPushConstantRange presentPushConstant{};
        CreatePushConstantRange(presentPushConstant, VK_SHADER_STAGE_COMPUTE_BIT, sizeof(BlitML::vec2));

        if (!CreatePipelineLayout(device, &context.m_presentLayout.handle, 1, &descriptorContext.m_presentSetlayout.handle, 1, &presentPushConstant))
        {
            BLIT_ERROR("Failed to create presentation generation pipeline layout");
            return 0;
        }

        // Success
        return 1;
    }

    uint8_t CreateReadWriteBuffers(VkDevice device, VmaAllocator vma, RWResources* readWritesArray, DescriptorContext& descriptorContext)
    {
        for (size_t frame = 0; frame < ce_framesInFlight; ++frame)
        {
            auto& readWrites = readWritesArray[frame];

            // Creates the uniform buffer for view data
            if (!CreateUBUFFER(vma, device, readWrites.m_viewDataBuffer, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
            {
                BLIT_ERROR("%s: Failed to create view data buffer", BLIT_VK_SYSTEM);
                return 0;
            }

            // Transform buffer is also dynamic
            Buffer transformStagingBufferTemp;
            auto transformBufferSize{ CreateSSBO<BlitzenEngine::MeshTransform>(vma, device, readWrites.m_transformBuffer.m_buffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                BLIT_MAX_WORLD_TRANSFORM_COUNT) };
            if (transformBufferSize == 0)
            {
                BLIT_ERROR("%s: Failed to create transform buffer", BLIT_VK_SYSTEM);
                return 0;
            }

            VkDeviceSize drawCmdBufferSize{ CreateSSBO<IndirectDrawData>(vma, device, readWrites.m_drawCmdBuffer,  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, 
                Ce_DrawCmdElementCount) };
            if (drawCmdBufferSize == 0)
            {
                BLIT_ERROR("%s: Failed to create indirect draw cmd buffer", BLIT_VK_SYSTEM);
                return 0;
            }

            VkDeviceSize drawCmdCounterSize{ CreateSSBO<uint32_t>(vma, device, readWrites.m_drawCmdCounterBuffer, 
                VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 1) };
            if (drawCmdCounterSize == 0)
            {
                BLIT_ERROR("%s: Failed to create indirect draw cmd counter buffer", BLIT_VK_SYSTEM);
                return 0;
            }

            VkDeviceSize visibilityBufferSize{ CreateSSBO<uint32_t>(vma, device, readWrites.m_drawVisBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                BLIT_MAX_WORLD_RENDERS) };
            if (visibilityBufferSize == 0)
            {
                BLIT_ERROR("%s: Failed to create draw visibility buffer", BLIT_VK_SYSTEM);
                return 0;
            }

            if (BlitzenCore::Ce_BuildClusters)
            {
                VkDeviceSize clusterGroupBufferSize{ CreateSSBO<ClusterGroupData>(vma, device, readWrites.m_clusterGroupDataBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, Ce_ClusterGroupBufferSize) };
                if (clusterGroupBufferSize == 0)
                {
                    BLIT_ERROR("%s: Failed to create cluster group data buffer", BLIT_VK_SYSTEM);
                    return 0;
                }

                VkDeviceSize clusterCounterBufferSize{ CreateSSBO<uint32_t>(vma, device, readWrites.m_clusterDispatchCounterBuffer,
                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 1) };
                if (clusterCounterBufferSize == 0)
                {
                    BLIT_ERROR("%s: Failed to create cluster dispatch counter", BLIT_VK_SYSTEM);
                    return 0;
                }

                if (!CreateBuffer(vma, readWrites.m_clusterDispatchCounterCopy.m_buffer, VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_CPU_ONLY, sizeof(uint32_t), VMA_ALLOCATION_CREATE_MAPPED_BIT))
                {
                    BLIT_ERROR("%s: Failed to create cluster counter copy", BLIT_VK_SYSTEM);
                    return 0;
                }
            }
        }

        return 1;
    }

    uint8_t CreateReadOnlyBuffers(VkDevice device, VmaAllocator vma, ROResources& readOnlies, VulkanStats& stats)
    {
        // Additional RT flags for geometry
        auto bRT{ stats.bRayTracingSupported };
        uint32_t geometryRtFlags = bRT ? VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT : 0;

        VkDeviceSize vertexBufferSize{ CreateSSBO<BlitzenEngine::Vertex>(vma, device, readOnlies.m_vtxBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | geometryRtFlags,
            BlitzenCore::Ce_MaxWorldVertexCount)};
        if (vertexBufferSize == 0)
        {
            BLIT_ERROR("%s: Failed to create vertex buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        VkDeviceSize indexBufferSize{ CreateSSBO<uint32_t>(vma, device, readOnlies.m_idxBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            BlitzenCore::Ce_MaxWorldVertexIndicesCount) };
        if (indexBufferSize == 0)
        {
            BLIT_ERROR("%s: Failed to create index buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        VkDeviceSize renderBufferSize{ CreateSSBO<BlitzenEngine::RenderObject>(vma, device, readOnlies.m_renderBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            BLIT_MAX_WORLD_RENDERS) };
        if (renderBufferSize == 0)
        {
            BLIT_ERROR("%s: Failed to create render object buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        VkDeviceSize boundingSphereBufferSize{ CreateSSBO<BlitzenEngine::BoundingSphere>(vma, device, readOnlies.m_boundingSphereBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            BlitzenEngine::CE_MAX_WORLD_BOUNDING_SPHERE_COUNT) };
        
        VkDeviceSize surfaceBufferSize{ CreateSSBO<BlitzenEngine::PrimitiveSurface>(vma, device, readOnlies.m_surfaceBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            BlitzenCore::Ce_MaxMeshPrimitivesCount) };
        if (surfaceBufferSize == 0)
        {
            BLIT_ERROR("%s: Failed to create mesh primitives buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        VkDeviceSize lodBufferSize{ CreateSSBO<BlitzenEngine::LodData>(vma, device, readOnlies.m_LODBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            BlitzenEngine::CE_MAX_LOD_COUNT)};
        if (lodBufferSize == 0)
        {
            BLIT_ERROR("%s: Failed to create LOD buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        VkDeviceSize materialBufferSize{ CreateSSBO<BlitzenEngine::Material>(vma, device, readOnlies.m_matBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
            BlitzenCore::Ce_MaxMaterialCount) };
        if (materialBufferSize == 0)
        {
            BLIT_ERROR("%s: Failed to create material buffer", BLIT_VK_SYSTEM);
            return 0;
        }

        if (BlitzenCore::Ce_BuildClusters)
        {
            VkDeviceSize clusterBufferSize = CreateSSBO<BlitzenEngine::Cluster>(vma, device,  readOnlies.m_clusterBuffer, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                BlitzenEngine::CE_MAX_WORLD_CLUSTER_COUNT);
            if (clusterBufferSize == 0)
            {
                BLIT_ERROR("%s: Failed to create cluster buffer", BLIT_VK_SYSTEM);
                return 0;
            }

            VkDeviceSize clusterIndexBufferSize = CreateSSBO<uint32_t>(vma, device, readOnlies.m_clusterIdxBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
                BlitzenCore::Ce_MaxWorldVertexIndicesCount);
            if (clusterIndexBufferSize == 0)
            {
                BLIT_ERROR("%s: Failed to create cluster indices buffer", BLIT_VK_SYSTEM);
                return 0;
            }
        }

        // SUCCESS
        return 1;
    }

    uint8_t CreateDummy2DTextureViews(VkDevice device, VmaAllocator vma, ROResources& readOnlies)
    {
        for (uint32_t i = 0; i < BlitzenCore::Ce_MaxTextureCount; ++i)
        {
            if (!Create2DImageResource(device, vma, readOnlies.m_textures[i].image, 1, 1, VK_FORMAT_BC1_RGBA_UNORM_BLOCK, VK_IMAGE_USAGE_SAMPLED_BIT, 1, VMA_MEMORY_USAGE_GPU_ONLY))
            {
                BLIT_ERROR("%s: Failed to create dummy 2DTex image view", BLIT_VK_SYSTEM);
                return 0;
            }

            readOnlies.m_textures[i].sampler = readOnlies.m_textureSampler.m_handle;
        }
        return 1;
    }
}