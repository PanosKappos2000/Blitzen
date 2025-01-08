#pragma once

#include "BlitzenMathLibrary/blitML.h"

namespace BlitzenEngine
{
    struct GameObject
    {
        uint32_t meshIndex;// Index into the mesh array inside of the LoadedResources struct in BlitzenEngine

        uint32_t transformIndex;// Index into the transform array in Engine resources

        BlitML::vec3 position;
        float scale;
        BlitML::quat orientation;
    };
}