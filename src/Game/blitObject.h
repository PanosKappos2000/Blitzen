#pragma once

#include "Core/blitLogger.h"

namespace BlitzenEngine
{
    // This game object can potentially allow a user of the engine to create a custom character 
    // by selecting one of the loaded meshes and giving it an entry into the transform array of the renderer
    struct GameObject
    {
        uint32_t meshIndex; // Index into the mesh array inside of the LoadedResources struct in BlitzenEngine

        // TODO: This could also be replaced with a reference to an instance of the struct, 
        // but including the file will cause circular dependency at the moment
        uint32_t transformIndex; // Index into the transform array in Engine resources,
        // used to access orientation, position and scale. But to also change them when necessary
    };
}