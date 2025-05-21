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
                # ENGINE CORE
                src/Core/blitzenEngine.h
                src/Core/blitMemory.h
                src/Core/blitLogger.h
                src/Core/blitzenLogger.cpp
                src/Core/blitAssert.h
                src/Core/blitKeys.h
                src/Core/blitEvents.h
                src/Core/blitzenEvents.cpp
                src/Core/blitTimeManager.h
                src/Core/blitzenTimeManager.cpp
                src/Core/blitzenWorld.h 
                src/Core/blitzenWorldPrivate.h
                src/Core/blitEntityManager.h
                src/Core/blitzenEntityManager.cpp
                src/Core/blitzenEntry.cpp

                # ENGINE RENDERING 
                src/Renderer/renderingResourcesTypes.h
                src/Renderer/blitRenderingResources.h
                src/Renderer/blitzenRenderingResources.cpp
                src/Renderer/Interface/blitRenderer.h
                src/Renderer/Interface/blitzenRenderer.cpp
                src/Renderer/Interface/blitRendererInterface.h
                src/Renderer/blitDDSTextures.h
                src/Renderer/blitzenDDSTextures.cpp
                # VULKAN
                src/Renderer/BlitzenVulkan/vulkanData.h
                src/Renderer/BlitzenVulkan/vulkanRenderer.h
                src/Renderer/BlitzenVulkan/vulkanResourceFunctions.h
                src/Renderer/BlitzenVulkan/vulkanCommands.h
                src/Renderer/BlitzenVulkan/vulkanPipelines.h
                src/Renderer/BlitzenVulkan/vulkanHandles.cpp
                src/Renderer/BlitzenVulkan/vulkanInit.cpp
                src/Renderer/BlitzenVulkan/vulkanResources.cpp
                src/Renderer/BlitzenVulkan/vulkanPipelines.cpp
                src/Renderer/BlitzenVulkan/vulkanRaytracing.cpp
                src/Renderer/BlitzenVulkan/vulkanRendererSetup.cpp
                src/Renderer/BlitzenVulkan/vulkanCommands.cpp
                src/Renderer/BlitzenVulkan/vulkanDraw.cpp
                # GL
                src/Renderer/BlitzenGL/openglData.h
                src/Renderer/BlitzenGL/openglRenderer.h
                src/Renderer/BlitzenGL/openglResources.cpp
                src/Renderer/BlitzenGl/openglRenderer.cpp
                # DX12
                src/Renderer/BlitzenDX12/dx12Data.h
                src/Renderer/BlitzenDX12/dx12Renderer.h
                src/Renderer/BlitzenDX12/dx12Commands.h 
                src/Renderer/BlitzenDX12/dx12Commands.cpp
                src/Renderer/BlitzenDX12/dx12Resources.h
                src/Renderer/BlitzenDX12/dx12Resources.cpp
                src/Renderer/BlitzenDX12/dx12Pipelines.h
                src/Renderer/BlitzenDX12/dx12Pipelines.cpp
                src/Renderer/BlitzenDX12/dx12RNDResources.h
                src/Renderer/BlitzenDX12/dx12RNDResources.cpp
                src/Renderer/BlitzenDX12/dx12Init.cpp
                src/Renderer/BlitzenDX12/dx12RendererSetup.cpp
                src/Renderer/BlitzenDX12/dx12Draw.cpp

                src/Game/blitObject.h
                src/Game/blitzenObject.cpp
                src/Game/blitCamera.h
                src/Game/blitzenCamera.cpp

                src/BlitCL/blitPfn.h
                src/BlitCL/blitzenContainerLibrary.h
                src/BlitCL/DynamicArray.h
                src/BlitCL/blitString.h
                src/BlitCL/blitSmartPointer.h

                src/BlitzenMathLibrary/blitML.h
                src/BlitzenMathLibrary/blitMLTypes.h
                
                src/Platform/platform.h
                src/Platform/blitzenWindows.cpp
                src/Platform/blitzenLinux.cpp
                src/Platform/filesystem.h
                src/Platform/filesystem.cpp
                
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

                src/Core/blitzenEngine.h
                src/Core/blitMemory.h
                src/Core/blitLogger.h
                src/Core/blitzenLogger.cpp
                src/Core/blitAssert.h
                src/Core/blitKeys.h
                src/Core/blitEvents.h
                src/Core/blitzenEvents.cpp
                src/Core/blitTimeManager.h
                src/Core/blitzenTimeManager.cpp
                src/Core/blitzenWorld.h 
                src/Core/blitzenWorldPrivate.h
                src/Core/blitEntityManager.h
                src/Core/blitzenEntityManager.cpp
                src/Core/blitzenEntry.cpp

                # ENGINE RENDERING 
                src/Renderer/renderingResourcesTypes.h
                src/Renderer/blitRenderingResources.h
                src/Renderer/blitzenRenderingResources.cpp
                src/Renderer/Interface/blitRenderer.h
                src/Renderer/Interface/blitzenRenderer.cpp
                src/Renderer/Interface/blitRendererInterface.h
                src/Renderer/blitDDSTextures.h
                src/Renderer/blitzenDDSTextures.cpp
                # VULKAN
                src/Renderer/BlitzenVulkan/vulkanData.h
                src/Renderer/BlitzenVulkan/vulkanRenderer.h
                src/Renderer/BlitzenVulkan/vulkanResourceFunctions.h
                src/Renderer/BlitzenVulkan/vulkanCommands.h
                src/Renderer/BlitzenVulkan/vulkanPipelines.h
                src/Renderer/BlitzenVulkan/vulkanHandles.cpp
                src/Renderer/BlitzenVulkan/vulkanInit.cpp
                src/Renderer/BlitzenVulkan/vulkanResources.cpp
                src/Renderer/BlitzenVulkan/vulkanPipelines.cpp
                src/Renderer/BlitzenVulkan/vulkanRaytracing.cpp
                src/Renderer/BlitzenVulkan/vulkanRendererSetup.cpp
                src/Renderer/BlitzenVulkan/vulkanCommands.cpp
                src/Renderer/BlitzenVulkan/vulkanDraw.cpp

                src/Game/blitObject.h
                src/Game/blitzenObject.cpp
                src/Game/blitCamera.h
                src/Game/blitzenCamera.cpp

                src/BlitCL/blitPfn.h
                src/BlitCL/blitzenContainerLibrary.h
                src/BlitCL/DynamicArray.h
                src/BlitCL/blitString.h
                src/BlitCL/blitSmartPointer.h

                src/BlitzenMathLibrary/blitML.h
                src/BlitzenMathLibrary/blitMLTypes.h
                
                src/Platform/platform.h
                src/Platform/blitzenWindows.cpp
                src/Platform/blitzenLinux.cpp
                src/Platform/filesystem.h
                src/Platform/filesystem.cpp
                
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

                            BLIT_REIN_SANT_ENG # Defines that the application is indeed Blitzen Engine (this should only be removed for tests)
                            BLIT_GDEV_EDT # Defines that the Engine is in Game Dev mode
                            BLIT_ASSERTIONS_ENABLED

                            BLIT_VK_FORCE # Forces Vulkan as the renderer backend
                            #BLIT_GL_LEGACY_OVERRIDE # Activates opengl (only if vulkan override is on)

                            #BLITZEN_CLUSTER_CULLING # Makes Blitzen create clusters and tells APIs to go into cluster mode
                            BLITZEN_DRAW_INSTANCED_CULLING
                            #BLIT_VSYNC #Enables vsync logic for all APIs
                            #LAMBDA_GAME_OBJECT_TEST # Switches the way game objects are updated (lambda vs inheritance)
                            BLIT_DYNAMIC_OBJECT_TEST # Creates 1'000 rotating kittens
                            #BLIT_DOUBLE_BUFFERING #Enables double buffering (DX12 ignores this, and activates it anyway)
                            #BLIT_RAYTRACING
                            #BLIT_MESH_SHADERS
                            #BLIT_DEPTH_PYRAMID_TEST

                            # Vulkan specific preprocessor macros
                            BLIT_VK_VALIDATION_LAYERS
                            BLIT_VK_SYNCHRONIZATION_VALIDATION

                            # Dx12 specific preprocessor macros
                            #DX12_ENABLE_GPU_BASED_VALIDATION # Activates Debug1 (might break api initialization)
                            DX12_OCCLUSION_DRAW_CULL
                            #DX12_TEMPORAL_DRAW_OCCLUSION
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
                        D3DCompiler 
                        #d3d12sdklayers.lib

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



# Copies the assets folder to the binary directory
add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Assets ${CMAKE_CURRENT_BINARY_DIR}/Assets)
add_dependencies(BlitzenEngine copy_assets)

# Copies the opengl glsl shaders to the binary directory
add_custom_target(copy_glsl_shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/GlslShaders ${CMAKE_CURRENT_BINARY_DIR}/GlslShaders)
add_dependencies(BlitzenEngine copy_glsl_shaders)

# Copies the legacy hlsl shaders to the binary directory
add_custom_target(copy_hlsl_shaders
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/HlslShadersLegacy ${CMAKE_CURRENT_BINARY_DIR}/HlslShadersLegacy)
add_dependencies(BlitzenEngine copy_hlsl_shaders)


# Sets shader compilers
IF(WIN32)
    find_program(GLSL_VALIDATOR glslangValidator HINTS $"{PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Bin")
    set(GLSL_VALIDATOR "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Bin/glslangValidator.exe")

    find_program(DXC dxc HINTS $"{PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Bin")
    set(DXC "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Bin/dxc.exe")
ELSEIF(UNIX)
    find_program(GLSL_VALIDATOR glslangValidator HINTS $"{PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/UnixBin")
    set(GLSL_VALIDATOR "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/UnixBin/glslangValidator")
ENDIF(WIN32)



# SPIRV (Vulkan)
# Finds all glsl shaders files
file(GLOB_RECURSE GLSL_SOURCE_FILES "VulkanShaders/*.glsl")
foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/VulkanShaders/${FILE_NAME}.spv")

    add_custom_command(OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/VulkanShaders/"
        COMMAND ${GLSL_VALIDATOR} -V --target-env vulkan1.3 ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)
add_custom_target(VulkanShaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(BlitzenEngine VulkanShaders)


IF(WIN32)# HLSL for Dx12 not needed in non windows builds
# HLSL
# Finds pixel shader files and compiles them
file(GLOB_RECURSE HLSL_PS_FILES "HlslShaders/PS/*.hlsl")
foreach(PS ${HLSL_PS_FILES})
    get_filename_component(FILE_NAME ${PS} NAME)
    set(BIN "${PROJECT_BINARY_DIR}/HlslShaders/PS/${FILE_NAME}.bin")

    add_custom_command(OUTPUT ${BIN}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/HlslShaders/PS/" 
        COMMAND DXC -T ps_6_6 -E main -Fo ${BIN} ${PS} 
        DEPENDS ${PS} 
        COMMENT "Compiling HLSL ps: ${FILE_NAME}")

    list(APPEND HLSL_BIN_FILES ${BIN})
endforeach(PS)

# Finds vertex shader files
file(GLOB_RECURSE HLSL_VS_FILES "HlslShaders/VS/*.hlsl")
foreach(VS ${HLSL_VS_FILES})
    get_filename_component(FILE_NAME ${VS} NAME)
    set(BIN "${PROJECT_BINARY_DIR}/HlslShaders/VS/${FILE_NAME}.bin")

    add_custom_command(OUTPUT ${BIN}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/HlslShaders/VS/" 
        COMMAND DXC -T vs_6_6 -E main -Fo ${BIN} ${VS} 
        DEPENDS ${VS} 
        COMMENT "Compiling HLSL vs: ${FILE_NAME}")

    list(APPEND HLSL_BIN_FILES ${BIN})
endforeach(VS)

# Finds vertex shader files
file(GLOB_RECURSE HLSL_CS_FILES "HlslShaders/CS/*.hlsl")
foreach(CS ${HLSL_CS_FILES})
    get_filename_component(FILE_NAME ${CS} NAME)
    set(BIN "${PROJECT_BINARY_DIR}/HlslShaders/CS/${FILE_NAME}.bin")

    add_custom_command(OUTPUT ${BIN}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/HlslShaders/CS/" 
        COMMAND DXC -T cs_6_6 -E csMain -Fo ${BIN} ${CS} 
        DEPENDS ${CS} 
        COMMENT "Compiling HLSL cs: ${FILE_NAME}")

    list(APPEND HLSL_BIN_FILES ${BIN})
endforeach(CS)

add_custom_target(HlslShaders DEPENDS ${HLSL_BIN_FILES})
add_dependencies(BlitzenEngine HlslShaders)

#  # HLSL(Vulkan)
#  file(GLOB_RECURSE HLSL_SOURCE_FILES
#      "HlslShaders/*.hlsl")
#  foreach(HLSL ${HLSL_SOURCE_FILES})
#  get_filename_component(FILE_NAME ${HLSL} NAME)
#  set(SPIRV "${PROJECT_BINARY_DIR}/VulkanShaders/${FILE_NAME}.spv")
#  add_custom_command(OUTPUT ${SPIRV}
#    COMMAND dxc -T ps_6_0 -E main -Fo ${HLSL}.spv ${CMAKE_SOURCE_DIR}/shaders/${shader_name}.hlsl
#    DEPENDS ${GLSL})
#    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
#add_custom_target(VkHLSL_shaders DEPENDS ${SPIRV_BINARY_FILES})
#add_dependencies(BlitzenEngine VkHLSL_shaders)
endif(WIN32)
  
# Copy the spir-v shaders to the target directory
add_custom_command(TARGET BlitzenEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BlitzenEngine>/VulkanShaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_BINARY_DIR}/VulkanShaders" "$<TARGET_FILE_DIR:BlitzenEngine>/VulkanShaders")

# Copy the spir-v shaders to the target directory
add_custom_command(TARGET BlitzenEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BlitzenEngine>/HlslShaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_BINARY_DIR}/HlslShaders" "$<TARGET_FILE_DIR:BlitzenEngine>/HlslShaders")

# Copy the assets to the target directory
add_custom_command(TARGET BlitzenEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BlitzenEngine>/Assets/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_BINARY_DIR}/Assets" "$<TARGET_FILE_DIR:BlitzenEngine>/Assets")

# Copy the glsl shaders to the target directory
add_custom_command(TARGET BlitzenEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BlitzenEngine>/GlslShaders"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_BINARY_DIR}/GlslShaders" "$<TARGET_FILE_DIR:BlitzenEngine>/GlslShaders")

# Copy the hlsl shaders to the target directory
add_custom_command(TARGET BlitzenEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BlitzenEngine>/HlslShadersLegacy"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_BINARY_DIR}/HlslShadersLegacy" "$<TARGET_FILE_DIR:BlitzenEngine>/HlslShadersLegacy")
