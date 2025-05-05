#pragma once
#include <stdio.h>
#include "Engine/blitzenEngine.h"
#include "Platform/filesystem.h"

namespace BlitzenEngine
{
    // Copied form https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-pixelformat
    struct DDS_PIXELFORMAT
    {
      unsigned int dwSize;
      unsigned int dwFlags;
      unsigned int dwFourCC;
      unsigned int dwRGBBitCount;
      unsigned int dwRBitMask;
      unsigned int dwGBitMask;
      unsigned int dwBBitMask;
      unsigned int dwABitMask;
    };

    // Copied from https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header
    struct DDS_HEADER
    {
      unsigned int dwSize;
      unsigned int dwFlags;
      unsigned int dwHeight;
      unsigned int dwWidth;
      unsigned int dwPitchOrLinearSize;
      unsigned int dwDepth;
      unsigned int dwMipMapCount;
      unsigned int dwReserved1[11];
      DDS_PIXELFORMAT ddspf;
      unsigned int dwCaps;
      unsigned int dwCaps2;
      unsigned int dwCaps3;
      unsigned int dwCaps4;
      unsigned int dwReserved2;
    };

    // Copied from https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-header-dxt10
    struct DDS_HEADER_DXT10
    {
      unsigned int dxgiFormat;
      unsigned int resourceDimension;
      unsigned int miscFlag;
      unsigned int arraySize;
      unsigned int miscFlags2;
    };

    // Taken from https://learn.microsoft.com/en-us/windows/win32/api/dxgiformat/ne-dxgiformat-dxgi_format
    enum DXGI_FORMAT
    {
	    DXGI_FORMAT_BC1_UNORM = 71,
	    DXGI_FORMAT_BC1_UNORM_SRGB = 72,
	    DXGI_FORMAT_BC2_UNORM = 74,
	    DXGI_FORMAT_BC2_UNORM_SRGB = 75,
	    DXGI_FORMAT_BC3_UNORM = 77,
	    DXGI_FORMAT_BC3_UNORM_SRGB = 78,
	    DXGI_FORMAT_BC4_UNORM = 80,
	    DXGI_FORMAT_BC4_SNORM = 81,
	    DXGI_FORMAT_BC5_UNORM = 83,
	    DXGI_FORMAT_BC5_SNORM = 84,
	    DXGI_FORMAT_BC6H_UF16 = 95,
	    DXGI_FORMAT_BC6H_SF16 = 96,
	    DXGI_FORMAT_BC7_UNORM = 98,
	    DXGI_FORMAT_BC7_UNORM_SRGB = 99,
    };

    inline unsigned int FourCC(const char (&str)[5])
    {
	      return (unsigned(str[0]) << 0) | (unsigned(str[1]) << 8) | (unsigned(str[2]) << 16) | (unsigned(str[3]) << 24);
    }

    uint8_t OpenDDSImageFile(const char* filepath, DDS_HEADER& header, DDS_HEADER_DXT10& header10, 
        BlitzenPlatform::FileHandle& handle);

    // Returns the amount of data needed to be allocated for the image data
    size_t GetDDSImageSizeBC(unsigned int width, unsigned int height, unsigned int levels,
        unsigned int blockSize);

    size_t GetDDSBlockSize(DDS_HEADER& header, DDS_HEADER_DXT10& header10);
}