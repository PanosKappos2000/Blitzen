#pragma once
#include "Core/blitLogger.h"

namespace BlitzenEngine
{
    struct TextureStats
    {
        int32_t textureWidth = 0;
        int32_t textureHeight = 0;
        int32_t textureChannels = 0;
        uint8_t* pTextureData;
    };

    uint8_t LoadTextureFromFile(const char* filename, TextureStats& stats);
}