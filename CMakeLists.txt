cmake_minimum_required(VERSION 3.10)

project (
    BlitzenEngine 
    VERSION 0
    LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED True)

# Blitzen Engine Windows
IF(WIN32)
add_executable(BlitzenEngine
                src/Engine/blitzenEngine.cpp
                src/Engine/blitzenEngine.h
                src/Engine/blitzenDefaultEvents.cpp

                src/BlitzenDX12/dx12Data.h
                src/BlitzenDX12/dx12Renderer.h
                src/BlitzenDX12/dx12Init.cpp
                src/BlitzenDX12/dx12RendererSetup.cpp
                src/BlitzenDX12/dx12Draw.cpp

                src/BlitzenVulkan/vulkanData.h
                src/BlitzenVulkan/vulkanRenderer.h
                src/BlitzenVulkan/vulkanHandles.cpp
                src/BlitzenVulkan/vulkanInit.cpp
                src/BlitzenVulkan/vulkanResources.cpp
                src/BlitzenVulkan/vulkanPipelines.cpp
                src/BlitzenVulkan/vulkanRendererSetup.cpp
                src/BlitzenVulkan/vulkanDraw.cpp

                src/BlitzenGL/openglData.h
                src/BlitzenGL/openglRenderer.h
                src/BlitzenGL/openglResources.cpp
                src/BlitzenGl/openglRenderer.cpp

                src/Renderer/blitRenderingResources.h
                src/Renderer/blitzenRenderingResources.cpp
                src/Renderer/blitRenderer.h
                src/Renderer/blitzenRenderer.cpp
                src/Renderer/blitDDSTextures.h
                src/Renderer/blitzenDDSTextures.cpp

                src/Game/blitObject.h
                src/Game/blitzenObject.cpp
                src/Game/blitCamera.h
                src/Game/blitzenCamera.cpp
                
                src/Core/blitMemory.h
                src/Core/blitLogger.h
                src/Core/blitzenLogger.cpp
                src/Core/blitAssert.h
                src/Core/blitEvents.h
                src/Core/blitzenEvents.cpp
                src/Core/blitTimeManager.h
                src/Core/blitzenTimeManager.cpp

                src/BlitCL/blitzenContainerLibrary.h
                src/BlitCL/DynamicArray.h
                src/BlitCL/blitString.h

                src/BlitzenMathLibrary/blitML.h
                src/BlitzenMathLibrary/blitMLTypes.h
                
                src/Platform/platform.h
                src/Platform/blitzenWindows.cpp
                src/Platform/blitzenLinux.cpp
                src/Platform/filesystem.h
                src/Platform/filesystem.cpp
                
                src/VendorCode/stb_image.h
                src/VendorCode/fast_obj.h
                src/VendorCode/objparser.cpp
                src/VendorCode/Meshoptimizer/indexgenerator.cpp
                src/VendorCode/Meshoptimizer/quantization.cpp
                src/VendorCode/Meshoptimizer/vcacheoptimizer.cpp
                src/VendorCode/Meshoptimizer/vfetchoptimizer.cpp
                src/VendorCode/Meshoptimizer/clusterizer.cpp
                src/VendorCode/Meshoptimizer/simplifier.cpp
                src/VendorCode/Cgltf/cgltf.h
)
ELSEIF(UNIX)
add_executable(BlitzenEngine
                src/Engine/blitzenEngine.cpp
                src/Engine/blitzenEngine.h
                src/Engine/blitzenDefaultEvents.cpp

                src/BlitzenVulkan/vulkanData.h
                src/BlitzenVulkan/vulkanRenderer.h
                src/BlitzenVulkan/vulkanInit.cpp
                src/BlitzenVulkan/vulkanHandles.cpp
                src/BlitzenVulkan/vulkanResources.cpp
                src/BlitzenVulkan/vulkanPipelines.cpp
                src/BlitzenVulkan/vulkanRendererSetup.cpp
                src/BlitzenVulkan/vulkanDraw.cpp

                src/Renderer/blitRenderingResources.h
                src/Renderer/blitzenRenderingResources.cpp
                src/Renderer/blitRenderer.h
                src/Renderer/blitzenRenderer.cpp
                src/Renderer/blitDDSTextures.h
                src/Renderer/blitzenDDSTextures.cpp

                src/Game/blitObject.h
                src/Game/blitzenObject.cpp
                src/Game/blitCamera.h
                src/Game/blitzenCamera.cpp
                
                src/Core/blitMemory.h
                src/Core/blitLogger.h
                src/Core/blitzenLogger.cpp
                src/Core/blitAssert.h
                src/Core/blitEvents.h
                src/Core/blitzenEvents.cpp
                src/Core/blitTimeManager.h
                src/Core/blitzenTimeManager.cpp

                src/BlitCL/blitzenContainerLibrary.h

                src/BlitzenMathLibrary/blitML.h
                src/BlitzenMathLibrary/blitMLTypes.h
                
                src/Platform/platform.h
                src/Platform/blitzenWindows.cpp
                src/Platform/blitzenLinux.cpp
                src/Platform/filesystem.h
                src/Platform/filesystem.cpp
                
                src/VendorCode/stb_image.h
                src/VendorCode/fast_obj.h
                src/VendorCode/objparser.cpp
                src/VendorCode/Meshoptimizer/indexgenerator.cpp
                src/VendorCode/Meshoptimizer/quantization.cpp
                src/VendorCode/Meshoptimizer/vcacheoptimizer.cpp
                src/VendorCode/Meshoptimizer/vfetchoptimizer.cpp
                src/VendorCode/Meshoptimizer/clusterizer.cpp
                src/VendorCode/Meshoptimizer/simplifier.cpp
                src/VendorCode/Cgltf/cgltf.h
                #src/VendorCode/volk/volk.c
)
ENDIF(WIN32)

# Specify directories that hold header files
target_include_directories(
    BlitzenEngine PUBLIC
            "${PROJECT_SOURCE_DIR}/src"
            "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/include"
            "${PROJECT_SOURCE_DIR}/ExternalDependencies"
            "${PROJECT_SOURCE_DIR}/src/VendorCode"
            "${PROJECT_SOURCE_DIR}/ExternalDependencies/Glew/include"
            "${PROJECT_SOURCE_DIR}/ExternalDependencies/Submodules/ImGui"
)

target_compile_definitions(BlitzenEngine PUBLIC
                            BLIT_REIN_SANT_ENG
                            #BLIT_GDEV_EDT

                            #BLITZEN_VULKAN_OVERRIDE
                            # General rendering preprocessor macros
                            BLITZEN_RENDERING_STRESS_TEST
                            #BLIT_VSYNC

                            # Vulkan specific preprocessor macros
                            BLITZEN_VULKAN # Never undef this, it's broken because of VulkanMemoryCrucials
                            BLIT_VK_VALIDATION_LAYERS
                            #BLIT_VK_RAYTRACING
                            BLIT_VK_SYNCHRONIZATION_VALIDATION

                            BLIT_DYNAMIC_OBJECT_TEST

                            # Opengl preprocessor marcors
                            #BLIT_GL_LEGACY_OVERRIDE

                            # Engine core preprocessor macros
                            BLIT_ASSERTIONS_ENABLED
                            )

# Linker file directories and libraries to link for linux and Windows
IF(WIN32)
    target_link_directories(BlitzenEngine PUBLIC
                        "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Lib"
                        "${PROJECT_SOURCE_DIR}/ExternalDependencies/Glew/lib/Release/x64")
    target_link_libraries(BlitzenEngine PUBLIC  
                        vulkan-1
                        D3d12
                        DXGI
                        glew32s
                        OpenGL32)
ELSEIF(UNIX)
    target_link_directories(BlitzenEngine PUBLIC
                        "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/UnixLib")
    target_link_libraries(BlitzenEngine PUBLIC
                        libvulkan.so.1
                        X11.so
                        xcb.so
                        libX11-xcb.so)
ENDIF(WIN32)



# Copy the assets folder to the binary directory
add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Assets ${CMAKE_CURRENT_BINARY_DIR}/Assets
)
add_dependencies(BlitzenEngine copy_assets)

# Copy the glsl shaders to the binary directory
add_custom_target(copy_glsl_shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/GlslShaders ${CMAKE_CURRENT_BINARY_DIR}/GlslShaders
)
add_dependencies(BlitzenEngine copy_glsl_shaders)


#---------------------------------------------------------------------------------------------------
#Compiling glsl Shaders to Spir-v
#

# Find the GLSL VALIDATOR binary
IF(WIN32)
    find_program(GLSL_VALIDATOR glslangValidator HINTS $"{PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Bin")
    set(GLSL_VALIDATOR "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Bin/glslangValidator.exe")
ELSEIF(UNIX)
    find_program(GLSL_VALIDATOR glslangValidator HINTS $"{PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/UnixBin")
    set(GLSL_VALIDATOR "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/UnixBin/glslangValidator")
ENDIF(WIN32)
  
  # Find all glsl shaders files
  file(GLOB_RECURSE GLSL_SOURCE_FILES
      "VulkanShaders/*.glsl"
      )
  
  foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/VulkanShaders/${FILE_NAME}.spv")
    add_custom_command(
      OUTPUT ${SPIRV}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/VulkanShaders/"
      COMMAND ${GLSL_VALIDATOR} -V --target-env vulkan1.3 ${GLSL} -o ${SPIRV}
      DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
  endforeach(GLSL)
  
  add_custom_target(
      VulkanShaders 
      DEPENDS ${SPIRV_BINARY_FILES}
      )
  
  add_dependencies(BlitzenEngine VulkanShaders)
  
  # Copy the spir-v shaders to the target directory
  add_custom_command(TARGET BlitzenEngine POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BlitzenEngine>/VulkanShaders/"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${PROJECT_BINARY_DIR}/VulkanShaders"
          "$<TARGET_FILE_DIR:BlitzenEngine>/VulkanShaders"
          )

    # Copy the assets to the target directory
    add_custom_command(TARGET BlitzenEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BlitzenEngine>/Assets/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/Assets"
        "$<TARGET_FILE_DIR:BlitzenEngine>/Assets"
      )

    # Copy the glsl shaders to the target directory
    add_custom_command(TARGET BlitzenEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BlitzenEngine>/GlslShaders"
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_BINARY_DIR}/GlslShaders"
        "$<TARGET_FILE_DIR:BlitzenEngine>/GlslShaders")
