cmake_minimum_required(VERSION 3.10)

project (BlitzenEngine VERSION 0 LANGUAGES C CXX)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED True)

# SOURCE
add_executable(BlitzenEngine
                # ENGINE CORE
                "src/Core/blitzenEngine.h"
                "src/Core/blitMemory.h"
                "src/Core/blitzenEntry.cpp"
                # LOGGING / DEBUG
                "src/Core/DbLog/blitLogger.h"
                "src/Core/DbLog/blitLogger.cpp"
                "src/Core/DbLog/blitAssert.h"
                # EVENTS
                "src/Core/Events/blitKeys.h"
                "src/Core/Events/blitEventType.h"
                "src/Core/Events/blitController.h"
                "src/Core/Events/blitEvents.h"
                "src/Core/Events/blitzenEvents.cpp"
                "src/Core/Events/blitTimeManager.h"
                "src/Core/Events/blitzenTimeManager.cpp"
                "src/Core/Events/blitEditorEvents.h"
                # WORLD (ALL SYSTEMS)
                "src/Core/BlitzenWorld/blitzenWorld.h"
                "src/Core/BlitzenWorld/blitzenWorldPrivate.h"
                "src/Core/BlitzenWorld/blitzenWorldLoad.cpp"
                "src/Core/BlitzenWorld/blitzenWorldUpdate.cpp"
                "src/Core/BlitzenWorld/blitzenWorld.cpp"
                # DASHER UI
                "src/Core/Dasher/Interface/dasherState.h"
                "src/Core/Dasher/Interface/dasherInterface.h"
                "src/Core/Dasher/DearDasher/dearDasherData.h"
                "src/Core/Dasher/DearDasher/dearDasher.h"
                "src/Core/Dasher/DearDasher/dearDasher.cpp"
                "src/Core/Dasher/DearDasher/dearDasherVK.h"
                "src/Core/Dasher/DearDasher/dearDasherVK.cpp"
                "src/Core/Dasher/DearDasher/dearDasherDX12.h" 
                "src/Core/Dasher/DearDasher/dearDasherDX12.cpp"
                

                # BLITZEN CONTAINER LIBRARY
                "src/BlitCL/blitPfn.h"
                "src/BlitCL/blitzenContainerLibrary.h"
                "src/BlitCL/DynamicArray.h"
                "src/BlitCL/blitString.h"
                "src/BlitCL/blitSmartPointer.h"
                "src/BlitCL/blitArray.h"
                "src/BlitCL/blitHashMap.h"
                "src/BlitCL/blitArrayIterator.h"
                "src/BlitCL/blitclDebug.h"
                "src/BlitCL/blitLinearAllocator.h"

                # BLITZEN MATH LIBRARY
                "src/BlitzenMathLibrary/blitML.h"
                "src/BlitzenMathLibrary/blitMLTypes.h"
                

                # PLATFORM
                "src/Platform/blitPlatformContext.h"
                "src/Platform/blitPlatform.h"
                "src/Platform/Common/blitMappedFile.h"
                # WINDOWS
                "src/Platform/BlitzenWindows/blitzenWin32.cpp"
                "src/Platform/BlitzenWindows/win32Memory.cpp"
                "src/Platform/BlitzenWindows/win32Clock.cpp"
                "src/Platform/BlitzenWindows/win32MappedFile.cpp"
                "src/Platform/BlitzenWindows/win32Log.cpp"
                "src/Platform/BlitzenWindows/win32GL.cpp"
                "src/Platform/BlitzenWindows/win32VK.cpp"
                # LINUX
                "src/Platform/BlitzenLinux/blitzenUnix.cpp"
                "src/Platform/BlitzenLinux/unixMemory.cpp"
                "src/Platform/BlitzenLinux/unixClock.cpp"
                "src/Platform/BlitzenLinux/unixMappedFile.cpp"
                "src/Platform/BlitzenLinux/unixLog.cpp"
                "src/Platform/BlitzenLinux/unixGL.cpp"
                "src/Platform/BlitzenLinux/unixVK.cpp"
                # OTHER
                "src/Platform/Filesystem/blitCFILE.h"
                "src/Platform/Filesystem/blitCFILE.cpp"


                # ENGINE RENDERING 
                "src/Renderer/Interface/blitRenderer.h"
                "src/Renderer/Interface/blitzenRenderer.cpp"
                "src/Renderer/Interface/blitRendererInterface.h"
                "src/Renderer/Resources/renderingResourcesTypes.h"
                "src/Renderer/Resources/blitRenderingResources.h"
                "src/Renderer/Resources/blitzenRenderingResources.cpp"
                "src/Renderer/Resources/Textures/blitTextures.h"
                "src/Renderer/Resources/Textures/blitzenTextures.cpp"
                "src/Renderer/Resources/Mesh/blitMeshes.h"
                "src/Renderer/Resources/Mesh/blitzenMeshes.cpp"
                "src/Renderer/Resources/Scene/blitScene.h"
                "src/Renderer/Resources/Scene/blitzenScene.cpp"
                "src/Renderer/Resources/Mesh/blitMeshPrimitive.h"
                "src/Core/BlitzenWorld/blitzenWorldRender.cpp"

                # CAMERA
                "src/Renderer/View/blitCamera.h"
                "src/Renderer/View/blitzenCamera.cpp"

                # ENTITIES
                "src/Renderer/Entities/DynamicTransform/blitDynamicTransform.h"
                "src/Renderer/Entities/DynamicTransform/blitDynamicTransform.cpp"
                "src/Renderer/Entities/Interface/blitEntityInterface.h"
                "src/Renderer/Entities/Interface/blitEntityManager.h"
                "src/Renderer/Entities/Interface/blitEntityInterface.cpp"
                "src/Renderer/Entities/Collision/blitCollision.h"
                "src/Renderer/Entities/Physics/blitGravity.h"
                "src/Renderer/Entities/Residents/blitWV.h"
                "src/Renderer/Entities/Residents/blitResident.h"
                "src/Renderer/Entities/Residents/blitResidentManager.h"
                "src/Renderer/Entities/Residents/RenderObject/blitRender.h"
                "src/Renderer/Entities/Residents/RenderObject/blitzenRender.cpp"
                "src/Renderer/Entities/Residents/blitResident.cpp"
                "src/Renderer/Entities/Residents/blitWV.cpp"
                "src/Renderer/Entities/Residents/RenderObject/worldTransform.h"

                # LIGHTING
                "src/Renderer/Lighting/blitDirectionalLight.h"

                # VULKAN
                "src/Renderer/BlitzenVulkan/vulkanData.h"
                "src/Renderer/BlitzenVulkan/vulkanContext.h"
                "src/Renderer/BlitzenVulkan/vulkanRenderer.h"
                "src/Renderer/BlitzenVulkan/vulkanResourceFunctions.h"
                "src/Renderer/BlitzenVulkan/vulkanRNDResources.h" 
                "src/Renderer/BlitzenVulkan/vulkanRNDResources.cpp"
                "src/Renderer/BlitzenVulkan/vulkanCommands.h"
                "src/Renderer/BlitzenVulkan/vulkanPipelines.h"
                "src/Renderer/BlitzenVulkan/vulkanHandles.cpp"
                "src/Renderer/BlitzenVulkan/vulkanInit.cpp"
                "src/Renderer/BlitzenVulkan/vulkanResources.cpp"
                "src/Renderer/BlitzenVulkan/vulkanPipelines.cpp"
                "src/Renderer/BlitzenVulkan/vulkanRaytracing.cpp"
                "src/Renderer/BlitzenVulkan/vulkanRendererSetup.cpp"
                "src/Renderer/BlitzenVulkan/vulkanCommands.cpp"
                "src/Renderer/BlitzenVulkan/vulkanRuntimeHelpers.cpp"
                "src/Renderer/BlitzenVulkan/vulkanDraw.cpp"
                "src/Renderer/BlitzenVulkan/Cull/vkCull.cpp"

                # GL
                "src/Renderer/BlitzenGL/openglData.h"
                "src/Renderer/BlitzenGL/openglRenderer.h"
                "src/Renderer/BlitzenGL/openglResources.cpp"
                "src/Renderer/BlitzenGL/openglRenderer.cpp"

                # DX12
                "src/Renderer/BlitzenDX12/dx12Data.h"
                "src/Renderer/BlitzenDX12/dx12Context.h"
                "src/Renderer/BlitzenDX12/dx12Renderer.h"
                "src/Renderer/BlitzenDX12/dx12Commands.h" 
                "src/Renderer/BlitzenDX12/dx12Commands.cpp"
                "src/Renderer/BlitzenDX12/dx12Resources.h"
                "src/Renderer/BlitzenDX12/dx12Resources.cpp"
                "src/Renderer/BlitzenDX12/dx12Pipelines.h"
                "src/Renderer/BlitzenDX12/dx12Pipelines.cpp"
                "src/Renderer/BlitzenDX12/dx12RNDResources.h"
                "src/Renderer/BlitzenDX12/dx12RNDResources.cpp"
                "src/Renderer/BlitzenDX12/dx12Init.cpp"
                "src/Renderer/BlitzenDX12/dx12RendererSetup.cpp"
                "src/Renderer/BlitzenDX12/dx12RuntimeHepers.cpp"
                "src/Renderer/BlitzenDX12/dx12Draw.cpp"
                "src/Renderer/BlitzenDX12/Update/dx12Update.h"
                "src/Renderer/BlitzenDX12/Update/dx12Update.cpp"
                "src/Renderer/BlitzenDX12/Init/dx12Init.h"
                "src/Renderer/BlitzenDX12/Cull/dx12Cull.cpp"

                "src/VendorCode/fast_obj.h"
                "src/VendorCode/objparser.cpp"
                "src/VendorCode/Meshoptimizer/indexgenerator.cpp"
                "src/VendorCode/Meshoptimizer/quantization.cpp"
                "src/VendorCode/Meshoptimizer/vcacheoptimizer.cpp"
                "src/VendorCode/Meshoptimizer/vfetchoptimizer.cpp"
                "src/VendorCode/Meshoptimizer/clusterizer.cpp"
                "src/VendorCode/Meshoptimizer/simplifier.cpp"
                "src/VendorCode/Cgltf/cgltf.h"                   
                
                
                
                "src/Client/WVRotatingKitten.h"   
                "src/Client/ClientWV/wvData.h" )

# INCLUDES
target_include_directories(BlitzenEngine PUBLIC
            "${PROJECT_SOURCE_DIR}/src"
            "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/include"
            "${PROJECT_SOURCE_DIR}/ExternalDependencies"
            "${PROJECT_SOURCE_DIR}/src/VendorCode"
            "${PROJECT_SOURCE_DIR}/ExternalDependencies/Glew/include"
            "${PROJECT_SOURCE_DIR}/ExternalDependencies/Submodules/ImGui"
)

# IMGUI
set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/ExternalDependencies/Submodules/ImGui")

target_sources(BlitzenEngine PUBLIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_tables.cpp  
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp  
)

# LINKER
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

target_compile_definitions(BlitzenEngine PUBLIC

                            BLIT_REIN_SANT_ENG # Defines that the application is indeed Blitzen Engine (this should only be removed for tests)
                            BLIT_GDEV_EDT # Activates standard main
                            BLIT_ASSERTIONS_ENABLED
                            BLIT_CONSOLE_LOGGER

                            DASHER_JOIN
                            DASHER_USE_DEAR

                            #BLIT_VK_FORCE # Forces Vulkan as the renderer backend
                            #BLIT_GL_LEGACY_OVERRIDE # Activates opengl (only on windows. It get overriden if VK_FORCE is defined)

                            #BLITZEN_CLUSTER_CULLING # Activates mesh cluster mode
                            #BLITZEN_DRAW_INSTANCED_CULLING # Activates indirect instancing
                            BLITZEN_DRAW_TEMPORAL_OCCLUSION
                            BLIT_VSYNC # VSYNC (DX12 is VSyned by default, the other two depend on this)
                            #LAMBDA_GAME_OBJECT_TEST # This is out of commision for now
                            BLIT_DYNAMIC_OBJECT_TEST # Creates 1'000 rotating kittens
                            #BLIT_DOUBLE_BUFFERING # Enables double buffering (DX12 ignores this, and activates it anyway)
                            #BLIT_RAYTRACING
                            #BLIT_MESH_SHADERS
                            #BLIT_DEPTH_PYRAMID_TEST # debug mode for HI-Z map

                            # Vulkan specific preprocessor macros
                            BLIT_VK_VALIDATION_LAYERS
                            BLIT_VK_SYNCHRONIZATION_VALIDATION

                            # Dx12 specific preprocessor macros
                            #DX12_ENABLE_GPU_BASED_VALIDATION # Activates Debug1 (might break api initialization)
                            #DX12_OCCLUSION_DRAW_CULL # Activates occlusion culling, with pre-pass
                            )



# Copies the assets folder to the binary directory
add_custom_target(copy_assets COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Assets ${CMAKE_CURRENT_BINARY_DIR}/Assets)
add_dependencies(BlitzenEngine copy_assets)

# Copies the opengl glsl shaders to the binary directory
add_custom_target(copy_glsl_shaders COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/src/Renderer/GlslShaders ${CMAKE_CURRENT_BINARY_DIR}/GlslShaders)
add_dependencies(BlitzenEngine copy_glsl_shaders)

# Copies the legacy hlsl shaders to the binary directory
add_custom_target(copy_hlsl_shaders COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/src/Renderer/HlslShadersLegacy ${CMAKE_CURRENT_BINARY_DIR}/HlslShadersLegacy)
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
# COMPUTE
file(GLOB_RECURSE GLSL_COMP_FILES "src/Renderer/VulkanShaders/Comp/*.glsl")
foreach(COMP ${GLSL_COMP_FILES})
    get_filename_component(FILE_NAME ${COMP} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/VulkanShaders/Comp/${FILE_NAME}.spv")

    add_custom_command(OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/VulkanShaders/Comp"
        COMMAND ${GLSL_VALIDATOR} -V --target-env vulkan1.3 ${COMP} -o ${SPIRV}
        DEPENDS ${COMP})

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(COMP)

# VERTEX
file(GLOB_RECURSE GLSL_VERT_FILES "src/Renderer/VulkanShaders/Vert/*.glsl")
foreach(VERT ${GLSL_VERT_FILES})
    get_filename_component(FILE_NAME ${VERT} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/VulkanShaders/Vert/${FILE_NAME}.spv")

    add_custom_command(OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/VulkanShaders/Vert"
        COMMAND ${GLSL_VALIDATOR} -V --target-env vulkan1.3 ${VERT} -o ${SPIRV}
        DEPENDS ${VERT})

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(VERT)

# FRAGMENT
file(GLOB_RECURSE GLSL_FRAG_FILES "src/Renderer/VulkanShaders/Frag/*.glsl")
foreach(FRAG ${GLSL_FRAG_FILES})
    get_filename_component(FILE_NAME ${FRAG} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/VulkanShaders/Frag/${FILE_NAME}.spv")

    add_custom_command(OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/VulkanShaders/Frag"
        COMMAND ${GLSL_VALIDATOR} -V --target-env vulkan1.3 ${FRAG} -o ${SPIRV}
        DEPENDS ${FRAG})

    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(FRAG)

add_custom_target(VulkanShaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(BlitzenEngine VulkanShaders)


# HLSL
IF(WIN32)# HLSL for Dx12 not needed in non windows builds
# Finds pixel shader files and compiles them
file(GLOB_RECURSE HLSL_PS_FILES "src/Renderer/HlslShaders/PS/*.hlsl")
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
file(GLOB_RECURSE HLSL_VS_FILES "src/Renderer/HlslShaders/VS/*.hlsl")
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
file(GLOB_RECURSE HLSL_CS_FILES "src/Renderer/HlslShaders/CS/*.hlsl")
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
# HLSL END


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

# Copy the HLSL shaders to the target directory
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
