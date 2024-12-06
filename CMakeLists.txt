cmake_minimum_required(VERSION 3.10)

project (BlitzenEngine VERSION 1.0)

find_package (Vulkan REQUIRED COMPONENTS glslc)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(BlitzenEngine
                src/mainEngine.cpp
                src/mainEngine.h
                src/Application/resourceLoading.h
                src/Application/resourceLoading.cpp
                
                src/Core/blitMemory.h
                src/Core/blitzenMemory.cpp
                src/Core/blitzenContainerLibrary.h
                src/Core/blitLogger.h
                src/Core/blitzenLogger.cpp
                src/Core/blitAssert.h
                src/Core/blitEvents.h
                src/Core/blitzenEvents.cpp

                src/BlitzenMathLibrary/blitML.h
                src/BlitzenMathLibrary/blitMLTypes.h

                src/BlitzenVulkan/vulkanRenderer.h
                src/BlitzenVulkan/vulkanRenderer.cpp
                src/BlitzenVulkan/vulkanInit.cpp
                src/BlitzenVulkan/vulkanData.h
                src/BlitzenVulkan/vulkanResources.cpp
                src/BlitzenVulkan/vulkanPipelines.cpp
                
                src/Platform/platform.h
                src/Platform/platform.cpp
                src/Platform/filesystem.h
                src/Platform/filesystem.cpp
                
                src/VendorCode/stb_image.h)

target_include_directories(BlitzenEngine PUBLIC
                        "${PROJECT_SOURCE_DIR}/src"
                        "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/include")

target_link_directories(BlitzenEngine PUBLIC
                        "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Lib")

IF(WIN32)
    target_link_libraries(BlitzenEngine PUBLIC  
                        vulkan-1)
ELSEIF(UNIX)
ENDIF(WIN32)



#Copy assets folder
add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Assets ${CMAKE_CURRENT_BINARY_DIR}/Assets
)
add_dependencies(BlitzenEngine copy_assets)


#---------------------------------------------------------------------------------------------------
#Compiling Shaders
#

find_program(GLSL_VALIDATOR glslangValidator HINTS $"{PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Bin")

set(GLSL_VALIDATOR "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Bin/glslangValidator.exe")
  
  
  file(GLOB_RECURSE GLSL_SOURCE_FILES
      "VulkanShaders/*.glsl"
      )
  
  foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/VulkanShaders/${FILE_NAME}.spv")
    add_custom_command(
      OUTPUT ${SPIRV}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/VulkanShaders/"
      COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
      DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
  endforeach(GLSL)
  
  add_custom_target(
      VulkanShaders 
      DEPENDS ${SPIRV_BINARY_FILES}
      )
  
  add_dependencies(BlitzenEngine VulkanShaders)
  
  add_custom_command(TARGET BlitzenEngine POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:BlitzenEngine>/VulkanShaders/"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${PROJECT_BINARY_DIR}/VulkanShaders"
          "$<TARGET_FILE_DIR:BlitzenEngine>/VulkanShaders"
          )