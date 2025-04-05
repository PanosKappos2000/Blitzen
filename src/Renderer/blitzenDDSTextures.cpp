#include "blitDDSTextures.h"

namespace BlitzenEngine
{
	uint8_t OpenDDSImageFile(const char* filepath, 
		DDS_HEADER& header, DDS_HEADER_DXT10& header10, 
		BlitzenPlatform::FileHandle& handle)
	{
		if (!handle.Open(filepath, BlitzenPlatform::FileModes::Read, 1))
			return 0;

		auto file = reinterpret_cast<FILE*>(handle.pHandle);

		unsigned int magic = 0;

		if (fread(&magic, sizeof(magic), 1, file) != 1 || magic != FourCC("DDS "))
			return 0;

		if (fread(&header, sizeof(header), 1, file) != 1)
			return 0;

		if (header.ddspf.dwFourCC == FourCC("DX10") && fread(&header10, sizeof(header10), 1, file) != 1)
			return 0;

		if (header.dwSize != sizeof(header) || header.ddspf.dwSize != sizeof(header.ddspf))
			return 0;

		if (header.dwCaps2 & (DDSCAPS2_CUBEMAP | DDSCAPS2_VOLUME))
			return 0;

		if (header.ddspf.dwFourCC == FourCC("DX10") && header10.resourceDimension != DDS_DIMENSION_TEXTURE2D)
			return 0;

		return 1;
	}

		/*switch (chosenRenderer)
		{
			case RendererToLoadDDS::Vulkan:
			{
				
			}

			case RendererToLoadDDS::Opengl:
			{
				
			}

			default:
				return 0;
		}
    }*/
}