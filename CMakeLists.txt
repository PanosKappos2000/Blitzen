cmake_minimum_required(VERSION 3.10)

project (BlitzenEngine VERSION 1.0)

find_package (Vulkan REQUIRED COMPONENTS glslc)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(BlitzenEngine
                src/mainEngine.cpp
                src/mainEngine.h
                
                src/Core/blitMemory.h
                src/Core/blitzenMemory.cpp
                src/Core/blitzenContainerLibrary.h
                src/Core/blitzenContainerLibrary.cpp
                src/Core/blitLogger.h
                src/Core/blitzenLogger.cpp
                src/Core/blitAssert.h
                
                src/Platform/platform.h
                src/Platform/platform.cpp)

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


#---------------------------------------------------------------------------------------------------
#Compiling Shaders
#

find_program(GLSL_VALIDATOR glslangValidator HINTS $"{PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Bin")

set(GLSL_VALIDATOR "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Bin/glslangValidator.exe")