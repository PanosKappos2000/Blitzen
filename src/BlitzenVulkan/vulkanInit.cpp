#include "vulkanRenderer.h"
#include "Platform/platform.h"
#include <cstring> // For strcmp

#if _MSC_VER
    #define VULKAN_SURFACE_KHR_EXTENSION_NAME       "VK_KHR_win32_surface"
    #define VALIDATION_LAYER_NAME                   "VK_LAYER_KHRONOS_validation"
#elif linux
    #define VULKAN_SURFACE_KHR_EXTENSION_NAME       "VK_KHR_xcb_surface"
    #define VALIDATION_LAYER_NAME                   "VK_LAYER_NV_optimus"
    #define VK_USE_PLATFORM_XCB_KHR
#endif

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




    uint8_t VulkanRenderer::Init(uint32_t windowWidth, uint32_t windowHeight)
    {
        /*
            If Blitzen ever uses a custom allocator it should be initalized here
        */
        m_pCustomAllocator = nullptr;



        /*------------------------
            VkInstance Creation
        -------------------------*/
        {
            // Like the assertion message below says, Blitzen will not use Vulkan without indirect
            BLIT_ASSERT_MESSAGE(BLITZEN_VULKAN_INDIRECT_DRAW, "Blitzen will not support Vulkan without draw indirect going forward.If you want to use Vulkan enable the BLITZEN_VULKAN_INDIRECT_DRAW on vulkanData.h")

            uint32_t apiVersion = 0;
            VK_CHECK(vkEnumerateInstanceVersion(&apiVersion));
            BLIT_ASSERT_MESSAGE(apiVersion > VK_API_VERSION_1_3, "Blitzen needs to use Vulkan API_VERSION 1.3")

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

            // Checking that all required extensions are supported
            uint32_t extensionsCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
            BlitCL::DynamicArray<VkExtensionProperties> availableExtensions(static_cast<size_t>(extensionsCount));
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableExtensions.Data());
            uint8_t extensionSupport[BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT] = {0};
            for(size_t i = 0; i < availableExtensions.GetSize(); ++i)
            {
                if(!strcmp(availableExtensions[i].extensionName,VULKAN_SURFACE_KHR_EXTENSION_NAME) && !extensionSupport[0])
                {
                    extensionSupport[0] = 1;
                }
                if(!strcmp(availableExtensions[i].extensionName, "VK_KHR_surface") && !extensionSupport[1])
                {
                    extensionSupport[1] = 1;
                }

                #if BLITZEN_VULKAN_VALIDATION_LAYERS
                    if(!strcmp(availableExtensions[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) && !extensionSupport[BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT - 1])
                        extensionSupport[BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT - 1] = 1;
                #endif
            }
            uint8_t allExtensions = 0;
            for(uint8_t i = 0; i < BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT; ++i)
            {
                allExtensions += extensionSupport[i];
            }
            BLIT_ASSERT_MESSAGE(allExtensions == BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT, "Not all extensions are supported")

            // Creating an array of required extension names to pass to ppEnabledExtensionNames
            const char* requiredExtensionNames [BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT];
            requiredExtensionNames[0] =  VULKAN_SURFACE_KHR_EXTENSION_NAME;
            requiredExtensionNames[1] = "VK_KHR_surface";        
            instanceInfo.enabledExtensionCount = BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT;
            instanceInfo.enabledLayerCount = 0; // Validation layers inactive at first, but will be activated if it's a debug build

            //If this is a debug build, the validation layer extension is also needed
            #if BLITZEN_VULKAN_VALIDATION_LAYERS

                requiredExtensionNames[BLITZEN_VULKAN_ENABLED_EXTENSION_COUNT - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

                // Getting all supported validation layers
                uint32_t availableLayerCount = 0;
                vkEnumerateInstanceLayerProperties(&availableLayerCount, nullptr);
                BlitCL::DynamicArray<VkLayerProperties> availableLayers(static_cast<size_t>(availableLayerCount));
                vkEnumerateInstanceLayerProperties(&availableLayerCount, availableLayers.Data());

                // Checking if the requested validation layers are supported
                uint8_t layersFound = 0;
                for(size_t i = 0; i < availableLayers.GetSize(); i++)
                {
                   if(!strcmp(availableLayers[i].layerName,VALIDATION_LAYER_NAME))
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

            VK_CHECK(vkCreateInstance(&instanceInfo, m_pCustomAllocator, &(m_initHandles.instance)))
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

        
        // Create the surface depending on the implementation on Platform.cpp
        BlitzenPlatform::CreateVulkanSurface(m_initHandles.instance, m_initHandles.surface, m_pCustomAllocator);




        
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
                if(!features.multiDrawIndirect || !features11.storageBuffer16BitAccess || !features11.shaderDrawParameters ||
                !features12.bufferDeviceAddress || !features12.descriptorIndexing || !features12.runtimeDescriptorArray ||  
                !features12.storageBuffer8BitAccess || !features12.shaderFloat16 || !features12.drawIndirectCount ||
                !features12.samplerFilterMinmax ||
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
                BlitCL::DynamicArray<VkQueueFamilyProperties2> queueFamilyProperties(static_cast<size_t>(queueFamilyPropertyCount));
                for(size_t j = 0; j < queueFamilyProperties.GetSize(); ++j)
                {
                    queueFamilyProperties[j].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
                }
                vkGetPhysicalDeviceQueueFamilyProperties2(pdv, &queueFamilyPropertyCount, queueFamilyProperties.Data());
                for(size_t j = 0; j < queueFamilyProperties.GetSize(); ++j)
                {
                    // Checks for a graphics queue index, if one has not already been found 
                    if(queueFamilyProperties[j].queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT && !m_graphicsQueue.hasIndex)
                    {
                        m_graphicsQueue.index = static_cast<uint32_t>(j);
                        m_graphicsQueue.hasIndex = 1;
                    }

                    // Checks for a compute queue index, if one has not already been found 
                    if(queueFamilyProperties[j].queueFamilyProperties.queueFlags & VK_QUEUE_COMPUTE_BIT && !m_computeQueue.hasIndex)
                    {
                        m_computeQueue.index = static_cast<uint32_t>(j);
                        m_computeQueue.hasIndex = 1;
                    }

                    VkBool32 supportsPresent = VK_FALSE;
                    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(pdv, static_cast<uint32_t>(j), m_initHandles.surface, &supportsPresent))
                    if(supportsPresent == VK_TRUE && !m_presentQueue.hasIndex)
                    {
                        m_presentQueue.index = static_cast<uint32_t>(j);
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

            if(!physicalDevices.GetSize())
            {
                BLIT_WARN("Your machine has no physical device that supports vulkan the way Blitzen wants it. \n \
                Try another graphics specification or disable some features")
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
                    m_initHandles.chosenGpu = pdv;
                    m_stats.hasDiscreteGPU = 1;
                }
            }

            // If a discrete GPU is not found, the renderer chooses the 1st device. This will change the way the renderer goes forward
            if(!m_stats.hasDiscreteGPU)
                m_initHandles.chosenGpu = physicalDevices[0];
            else
                BLIT_INFO("Discrete GPU found")

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

            // Since mesh shaders are not a required feature, they will be checked separately and if support is not found, the traditional pipeline will be used
            #if BLITZEN_VULKAN_MESH_SHADER
                VkPhysicalDeviceFeatures2 features2{};
                features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
                VkPhysicalDeviceMeshShaderFeaturesEXT meshFeatures{};
                meshFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT;
                features2.pNext = &meshFeatures;
                vkGetPhysicalDeviceFeatures2(m_initHandles.chosenGpu, &features2);
                m_stats.meshShaderSupport = meshFeatures.meshShader && meshFeatures.taskShader;

                // Check the extensions as well
                uint32_t dvExtensionCount = 0;
                vkEnumerateDeviceExtensionProperties(m_initHandles.chosenGpu, nullptr, &dvExtensionCount, nullptr);
                BlitCL::DynamicArray<VkExtensionProperties> dvExtensionsProps(static_cast<size_t>(dvExtensionCount));
                vkEnumerateDeviceExtensionProperties(m_initHandles.chosenGpu, nullptr, &dvExtensionCount, dvExtensionsProps.Data());
                uint8_t meshShaderExtension = 0;
                for(size_t i = 0; i < dvExtensionsProps.GetSize(); ++i)
                {
                    if(!strcmp(dvExtensionsProps[i].extensionName, VK_EXT_MESH_SHADER_EXTENSION_NAME))
                        meshShaderExtension = 1;
                }
                m_stats.meshShaderSupport = m_stats.meshShaderSupport && meshShaderExtension;

                if(m_stats.meshShaderSupport)
                    BLIT_INFO("Mesh shader support confirmed")
                else
                    BLIT_INFO("No mesh shader support, using traditional pipeline")
            #endif

            // Vulkan should ignore the mesh shader extension if support for it was not found
            deviceInfo.enabledExtensionCount = 2 + (BLITZEN_VULKAN_MESH_SHADER && m_stats.meshShaderSupport);
            // Adding the swapchain extension and mesh shader extension if it was requested
            const char* extensionsNames[2 + BLITZEN_VULKAN_MESH_SHADER];
            extensionsNames[0] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
            extensionsNames[1] = VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME;
            #if BLITZEN_VULKAN_MESH_SHADER
                if(m_stats.meshShaderSupport)
                    extensionsNames[2] = VK_NV_MESH_SHADER_EXTENSION_NAME;
            #endif
            deviceInfo.ppEnabledExtensionNames = extensionsNames;

            // Standard device features
            VkPhysicalDeviceFeatures deviceFeatures{};
            // Will allow the renderer to make one draw call for multiple objects using vkCmdDrawIndirect or the indexed version
            deviceFeatures.multiDrawIndirect = true;
            deviceInfo.pEnabledFeatures = nullptr;// Enabled features will be given to VkPhysicalDeviceFeatures2 and passed to the pNext chain

            // Extended device features
            VkPhysicalDeviceVulkan11Features vulkan11Features{};
            vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
            vulkan11Features.shaderDrawParameters = true;
            // Allows use of 16 bit types inside storage buffers in the shaders
            vulkan11Features.storageBuffer16BitAccess = true;

            VkPhysicalDeviceVulkan12Features vulkan12Features{};
            vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
            // Allows to use vkGetBufferDeviceAddress to get a pointer to a buffer that can be given to the shaders
            vulkan12Features.bufferDeviceAddress = true;
            vulkan12Features.descriptorIndexing = true;
            // Allows shaders to use array with undefined size for descriptors, needed for textures
            vulkan12Features.runtimeDescriptorArray = true;
            vulkan12Features.shaderFloat16 = true;
            vulkan12Features.storageBuffer8BitAccess = true;
            vulkan12Features.drawIndirectCount = true;
            vulkan12Features.samplerFilterMinmax = true;

            VkPhysicalDeviceVulkan13Features vulkan13Features{};
            vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
            // Using dynamic rendering to make things slightly easier(Get rid of render passes and framebuffer, allows definition of rendering attachments separately)
            vulkan13Features.dynamicRendering = true;
            vulkan13Features.synchronization2 = true;
            vulkan13Features.maintenance4 = true;

            VkPhysicalDeviceMeshShaderFeaturesNV vulkanFeaturesMesh{};
            vulkanFeaturesMesh.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_NV;
            #if BLITZEN_VULKAN_MESH_SHADER
                if(m_stats.meshShaderSupport)
                {
                    vulkanFeaturesMesh.meshShader = true;
                    vulkanFeaturesMesh.taskShader = true;
                }
            #endif

            VkPhysicalDeviceFeatures2 vulkanExtendedFeatures{};
            vulkanExtendedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
            vulkanExtendedFeatures.features = deviceFeatures;

            deviceInfo.pNext = &vulkanExtendedFeatures;
            vulkanExtendedFeatures.pNext = &vulkan11Features;
            vulkan11Features.pNext = &vulkan12Features;
            vulkan12Features.pNext = &vulkan13Features;
            vulkan13Features.pNext = &vulkanFeaturesMesh;

            BlitCL::DynamicArray<VkDeviceQueueCreateInfo> queueInfos(1);
            queueInfos[0].queueFamilyIndex = m_graphicsQueue.index;
            VkDeviceQueueCreateInfo deviceQueueInfo{};
            // If compute has a different index from present, add a new info for it
            if(m_graphicsQueue.index != m_computeQueue.index)
            {
                queueInfos.PushBack(deviceQueueInfo);
                queueInfos[1].queueFamilyIndex = m_computeQueue.index;
            }
            // If an info was created for compute and present is not equal to compute or graphics, create a new one for present as well
            if(queueInfos.GetSize() == 2 && queueInfos[0].queueFamilyIndex != m_presentQueue.index && queueInfos[1].queueFamilyIndex != m_presentQueue.index)
            {
                queueInfos.PushBack(deviceQueueInfo);
                queueInfos[2].queueFamilyIndex = m_presentQueue.index;
            }
            // If an info was not created for compute but present has a different index from the other 2, create a new info for it
            if(queueInfos.GetSize() == 1 && queueInfos[0].queueFamilyIndex != m_presentQueue.index)
            {
                queueInfos.PushBack(deviceQueueInfo);
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
        CreateSwapchain(m_device, m_initHandles, windowWidth, windowHeight, m_graphicsQueue, m_presentQueue, m_computeQueue, m_pCustomAllocator, 
        m_initHandles.swapchain);

        /* 
            Create the vma allocator for vulkan resource allocation 
        */
        VmaAllocatorCreateInfo allocatorInfo{};
        allocatorInfo.device = m_device;
        allocatorInfo.instance = m_initHandles.instance;
        allocatorInfo.physicalDevice = m_initHandles.chosenGpu;
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        VK_CHECK(vmaCreateAllocator(&allocatorInfo, &m_allocator));
        



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

        // Create the sync structure and command buffers that need to differ for each frame in flight
        FrameToolsInit();

        // This will be referred to by rendering attachments and will be updated when the window is resized
        m_drawExtent = {windowWidth, windowHeight};

        // Initlize the rendering attachments
        CreateImage(m_device, m_allocator, m_colorAttachment, {m_drawExtent.width, m_drawExtent.height, 1}, VK_FORMAT_R16G16B16A16_SFLOAT, 
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        CreateImage(m_device, m_allocator, m_depthAttachment, {m_drawExtent.width, m_drawExtent.height, 1}, VK_FORMAT_D32_SFLOAT, 
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        CreateDepthPyramid(m_depthPyramid, m_depthPyramidExtent, m_depthPyramidMips, m_depthPyramidMipLevels, m_depthAttachmentSampler, 
        m_drawExtent, m_device, m_allocator);

        return 1;
    }

    void CreateDepthPyramid(AllocatedImage& depthPyramidImage, VkExtent2D& depthPyramidExtent, 
    VkImageView* depthPyramidMips, uint8_t& depthPyramidMipLevels, VkSampler& depthAttachmentSampler, 
    VkExtent2D drawExtent, VkDevice device, VmaAllocator allocator, uint8_t createSampler /* =1 */)
    {
        // This will fail if mip levels do not start from 0
        depthPyramidMipLevels = 0;

        // Create the sampler only if it is requested. In case where the depth pyramid is recreated on window resize, this is not needed
        if(createSampler)
            depthAttachmentSampler = CreateSampler(device, VK_SAMPLER_REDUCTION_MODE_MIN);

        // Get a conservative starting extent for the depth pyramid image, by getting the previous power of 2 of the draw extent
        depthPyramidExtent.width = BlitML::PreviousPow2(drawExtent.width);
        depthPyramidExtent.height = BlitML::PreviousPow2(drawExtent.height);

        // Get the amount of mip levels for the depth pyramid by dividing the extent by 2, until both width and height are not over 1
        uint32_t width = depthPyramidExtent.width;
        uint32_t height = depthPyramidExtent.height;
        while(width > 1 || height > 1)
        {
            depthPyramidMipLevels++;
            width /= 2;
            height /= 2;
        }

        // Create the depth pyramid image which will be used as storage image and to then transfer its data for occlusion culling
        CreateImage(device, allocator, depthPyramidImage, {depthPyramidExtent.width, depthPyramidExtent.height, 1}, VK_FORMAT_R32_SFLOAT, 
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, depthPyramidMipLevels);

        for(uint8_t i = 0; i < depthPyramidMipLevels; ++i)
        {
            CreateImageView(device, depthPyramidMips[size_t(i)], depthPyramidImage.image, VK_FORMAT_R32_SFLOAT, i, 1);
        }
    }

    void CreateSwapchain(VkDevice device, InitializationHandles& initHandles, uint32_t windowWidth, uint32_t windowHeight, 
    Queue graphicsQueue, Queue presentQueue, Queue computeQueue, VkAllocationCallbacks* pCustomAllocator, VkSwapchainKHR& newSwapchain, 
    VkSwapchainKHR oldSwapchain /*=VK_NULL_HANDLE*/)
    {
            VkSwapchainCreateInfoKHR swapchainInfo{};
            swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            swapchainInfo.pNext = nullptr;
            swapchainInfo.flags = 0;
            swapchainInfo.imageArrayLayers = 1;
            swapchainInfo.clipped = VK_TRUE;// Don't present things renderer out of bounds
            swapchainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;// Might not be supported in some cases, so I might want to guard against this
            swapchainInfo.surface = initHandles.surface;
            swapchainInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            // Used when the swapchain is recreated
            swapchainInfo.oldSwapchain = oldSwapchain;

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

            // Set the swapchain extent to the window's width and height
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
            VK_CHECK(vkCreateSwapchainKHR(device, &swapchainInfo, pCustomAllocator, &newSwapchain));

            // Retrieve the swapchain images
            uint32_t swapchainImageCount = 0;
            VK_CHECK(vkGetSwapchainImagesKHR(device, newSwapchain, &swapchainImageCount, nullptr));
            initHandles.swapchainImages.Resize(swapchainImageCount);
            VK_CHECK(vkGetSwapchainImagesKHR(device, newSwapchain, &swapchainImageCount, initHandles.swapchainImages.Data()));
    }

    void VulkanRenderer::FrameToolsInit()
    {
        VkCommandPoolCreateInfo commandPoolsInfo {};
        commandPoolsInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolsInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Allow each individual command buffer to be reset
        commandPoolsInfo.queueFamilyIndex = m_graphicsQueue.index;

        VkCommandBufferAllocateInfo commandBuffersInfo{};
        commandBuffersInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBuffersInfo.pNext = nullptr;
        commandBuffersInfo.commandBufferCount = 1;
        commandBuffersInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        fenceInfo.pNext = nullptr;

        VkSemaphoreCreateInfo semaphoresInfo{};
        semaphoresInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphoresInfo.flags = 0;
        semaphoresInfo.pNext = nullptr;

        for(size_t i = 0; i < BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT; ++i)
        {
            FrameTools& frameTools = m_frameToolsList[i];
            VK_CHECK(vkCreateCommandPool(m_device, &commandPoolsInfo, m_pCustomAllocator, &(frameTools.mainCommandPool)));
            commandBuffersInfo.commandPool = frameTools.mainCommandPool;
            VK_CHECK(vkAllocateCommandBuffers(m_device, &commandBuffersInfo, &(frameTools.commandBuffer)));

            VK_CHECK(vkCreateFence(m_device, &fenceInfo, m_pCustomAllocator, &(frameTools.inFlightFence)))
            VK_CHECK(vkCreateSemaphore(m_device, &semaphoresInfo, m_pCustomAllocator, &(frameTools.imageAcquiredSemaphore)))
            VK_CHECK(vkCreateSemaphore(m_device, &semaphoresInfo, m_pCustomAllocator, &(frameTools.readyToPresentSemaphore)))
        }
    }


    void VulkanRenderer::Shutdown()
    {
        vkDeviceWaitIdle(m_device);

        vkDestroySampler(m_device, m_placeholderSampler, m_pCustomAllocator);

        m_currentStaticBuffers.Cleanup(m_allocator, m_device);

        vkDestroyDescriptorSetLayout(m_device, m_pushDescriptorBufferLayout, m_pCustomAllocator);

        vkDestroyPipeline(m_device, m_lateCullingComputePipeline, m_pCustomAllocator);
        vkDestroyPipelineLayout(m_device, m_lateCullingPipelineLayout, m_pCustomAllocator);

        vkDestroyPipeline(m_device, m_indirectCullingComputePipeline, m_pCustomAllocator);

        vkDestroyPipeline(m_device, m_opaqueGraphicsPipeline, m_pCustomAllocator);
        vkDestroyPipelineLayout(m_device, m_opaqueGraphicsPipelineLayout, m_pCustomAllocator);

        vkDestroyPipeline(m_device, m_depthReduceComputePipeline, m_pCustomAllocator);
        vkDestroyPipelineLayout(m_device, m_depthReducePipelineLayout, m_pCustomAllocator);
        vkDestroyDescriptorSetLayout(m_device, m_depthPyramidDescriptorLayout, m_pCustomAllocator);

        m_depthAttachment.CleanupResources(m_allocator, m_device);
        m_colorAttachment.CleanupResources(m_allocator, m_device);
        m_depthPyramid.CleanupResources(m_allocator, m_device);
        for(size_t i = 0; i < m_depthPyramidMipLevels; ++i)
        {
            vkDestroyImageView(m_device, m_depthPyramidMips[i], m_pCustomAllocator);
        }
        vkDestroySampler(m_device, m_depthAttachmentSampler, m_pCustomAllocator);

        for(size_t i = 0; i < BLITZEN_VULKAN_MAX_FRAMES_IN_FLIGHT; ++i)
        {
            FrameTools& frameTools = m_frameToolsList[i];
            VarBuffers& varBuffers = m_varBuffers[i];

            vkDestroyCommandPool(m_device, frameTools.mainCommandPool, m_pCustomAllocator);

            vkDestroyFence(m_device, frameTools.inFlightFence, m_pCustomAllocator);
            vkDestroySemaphore(m_device, frameTools.imageAcquiredSemaphore, m_pCustomAllocator);
            vkDestroySemaphore(m_device, frameTools.readyToPresentSemaphore, m_pCustomAllocator);

            vmaDestroyBuffer(m_allocator, varBuffers.cullingDataBuffer.buffer, varBuffers.cullingDataBuffer.allocation);
            vmaDestroyBuffer(m_allocator, varBuffers.globalShaderDataBuffer.buffer, varBuffers.globalShaderDataBuffer.allocation);
            vmaDestroyBuffer(m_allocator, varBuffers.bufferDeviceAddrsBuffer.buffer, varBuffers.bufferDeviceAddrsBuffer.allocation);
        }

        vkDestroyCommandPool(m_device, m_placeholderCommandPool, m_pCustomAllocator);

        vkDestroySwapchainKHR(m_device, m_initHandles.swapchain, m_pCustomAllocator);

        vmaDestroyAllocator(m_allocator);

        vkDestroyDevice(m_device, m_pCustomAllocator);
        vkDestroySurfaceKHR(m_initHandles.instance, m_initHandles.surface, m_pCustomAllocator);
        DestroyDebugUtilsMessengerEXT(m_initHandles.instance, m_initHandles.debugMessenger, m_pCustomAllocator);
        vkDestroyInstance(m_initHandles.instance, m_pCustomAllocator);
    }
}