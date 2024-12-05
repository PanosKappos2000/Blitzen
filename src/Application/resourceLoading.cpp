#include "textures.h"
#define STB_IMAGE_IMPLEMENTATION
#include "VendorCode/stb_image.h"

namespace BlitzenEngine
{
    uint8_t LoadTextureFromFile(const char* filename, TextureStats& stats)
    {
        stats.pTextureData = stbi_load(filename, &(stats.textureWidth), &(stats.textureHeight), &(stats.textureChannels), STBI_rgb_alpha);
        return stats.pTextureData != nullptr;
    }
}