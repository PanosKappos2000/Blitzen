cmake_minimum_required(VERSION 3.10)

project (BlitzenEngine VERSION 1.0)

find_package (Vulkan REQUIRED COMPONENTS glslc)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(BlitzenEngine
                src/mainEngine.cpp
                src/mainEngine.h
                
                src/Core/blitLogger.h
                src/Core/blitzenLogger.cpp
                src/Core/blitAssert.h)

target_link_directories(BlitzenEngine PUBLIC
                        "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Lib")

target_link_libraries(BlitzenEngine PUBLIC  
                        vulkan-1)


#---------------------------------------------------------------------------------------------------
#Compiling Shaders
#

find_program(GLSL_VALIDATOR glslangValidator HINTS $"{PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Bin")

set(GLSL_VALIDATOR "${PROJECT_SOURCE_DIR}/ExternalDependencies/Vulkan/Bin/glslangValidator.exe")