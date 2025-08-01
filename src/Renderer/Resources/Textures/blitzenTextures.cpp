#include "blitTextures.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenEngine
{
	uint8_t OpenDDSImageFile(const char* filepath, DDS_HEADER& header, DDS_HEADER_DXT10& header10, BlitzenPlatform::C_FILE_SCOPE& scopedFILE)
	{
		if (!scopedFILE.Open(filepath, BlitzenPlatform::FileModes::Read, 1))
		{
			BLIT_ERROR("%s: Failed to open DDS texture file", BlitzenCore::GCRenderingResourceSystemName);
			return 0;
		}

		auto file = scopedFILE.m_pHandle;

		uint32_t DDS_MAGIC_SIGNATURE = 0;

		// Reads and checks the DDS magic header to confirm the file is a valid DDS file
		if (fread(&DDS_MAGIC_SIGNATURE, sizeof(DDS_MAGIC_SIGNATURE), 1, file) != 1 || DDS_MAGIC_SIGNATURE != FourCC("DDS "))
		{
			BLIT_ERROR("%s: Invalid DDS signature in file: %s", BlitzenCore::GCRenderingResourceSystemName, filepath);
			return 0;
		}

		// Reads the main DDS header into the 'header' structure
		if (fread(&header, sizeof(header), 1, file) != 1)
		{
			BLIT_ERROR("Failed to read DDS header data from file: %s", filepath);
			return 0;
		}

		// If the DDS header indicates it's a DX10 format, reads the extended header (header10) data
		if (header.ddspf.dwFourCC == FourCC("DX10") && fread(&header10, sizeof(header10), 1, file) != 1)
		{
			BLIT_ERROR("Failed to readd DDS header10 data from file: %s", filepath);
			return 0;
		}

		// Check the sizes of the header and the ddspf structure to ensure they match the expected values
		if (header.dwSize != sizeof(header) || header.ddspf.dwSize != sizeof(header.ddspf))
		{
			BLIT_ERROR("Invalid DDS header size for file: %s", filepath);
			return 0;
		}

		// Checks if the DDS file is a cubemap or volume texture. Those types will get different dedicated functions later.
		if (header.dwCaps2 & (BlitzenCore::DDSCAPS2_CUBEMAP | BlitzenCore::DDSCAPS2_VOLUME))
		{
			return 0;
		}

		// If the DDS file is using the DX10 format, ensures it's a 2D texture. 3D textures might be handled elsewhere when they are needed
		if (header.ddspf.dwFourCC == FourCC("DX10") && header10.resourceDimension != BlitzenCore::DDS_DIMENSION_TEXTURE2D)
		{
			return 0;
		}

		return 1;
	}

	size_t GetDDSImageSizeBC(uint32_t width, uint32_t height, uint32_t levels, uint32_t blockSize)
	{
		size_t result = 0;
		for (uint32_t i = 0; i < levels; ++i)
		{
			result += ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
			width = width > 1 ? width / 2 : 1;
			height = height > 1 ? height / 2 : 1;
		}
		return result;
	}

	uint32_t GetDDSBlockSize(DDS_HEADER& header, DDS_HEADER_DXT10& header10)
	{
		if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DXT1"))
		{
			return 8;
		}
		if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DXT3"))
		{
			return 16;
		}
		if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DXT5"))
		{
			return 16;
		}

		if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DX10"))
		{
			switch ((BLIT_DXGI_FORMAT_COPY)header10.dxgiFormat)
			{
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC1_UNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC1_UNORM_SRGB:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC4_UNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC4_SNORM:
			{
				return 8;
			}

			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC2_UNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC2_UNORM_SRGB:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC3_UNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC3_UNORM_SRGB:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC5_UNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC5_SNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC6H_UF16:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC6H_SF16:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC7_UNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC7_UNORM_SRGB:
			{
				return 16;
			}

			default:
			{
				BLIT_ERROR("Unexpected texture format");
				return 16;
			}
			}
		}

		return 16;
	}

	static BLIT_DXGI_FORMAT_COPY GetDDSFormat(const BlitzenEngine::DDS_HEADER& header, const BlitzenEngine::DDS_HEADER_DXT10& header10)
	{
		if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DXT1"))
		{
			//return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			return BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC1_UNORM;

		}
		if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DXT3"))
		{
			//return VK_FORMAT_BC2_UNORM_BLOCK;
			return BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC2_UNORM;
		}
		if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DXT5"))
		{
			//return VK_FORMAT_BC3_UNORM_BLOCK;
			return BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC3_UNORM;
		}

		if (header.ddspf.dwFourCC == BlitzenEngine::FourCC("DX10"))
		{
			switch ((BLIT_DXGI_FORMAT_COPY)header10.dxgiFormat)
			{
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC1_UNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC1_UNORM_SRGB:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC2_UNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC2_UNORM_SRGB:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC3_UNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC3_UNORM_SRGB:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC4_UNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC4_SNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC5_UNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC5_SNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC6H_UF16:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC6H_SF16:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC7_UNORM:
			case BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_BC7_UNORM_SRGB:
			{
				return (BLIT_DXGI_FORMAT_COPY)header10.dxgiFormat;
			}
			}
		}

		return BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_UNKNOWN;
	}

	size_t LoadDDSImageData(DDSFileContext& context, BlitzenPlatform::C_FILE_SCOPE& scopedFILE, void* pData, const char* filepath)
	{
		if (!OpenDDSImageFile(filepath, context.mDDSHeader, context.mDDSHeader10, scopedFILE))
		{
			BLIT_ERROR("%s: Failed to open texture file", BlitzenCore::GCRenderingResourceSystemName);
			return 0;
		}

		context.mFormat = GetDDSFormat(context.mDDSHeader, context.mDDSHeader10);
		if (context.mFormat == BLIT_DXGI_FORMAT_COPY::DXGI_FORMAT_UNKNOWN)
		{
			BLIT_ERROR("%s: Unknown format retrieved from DDS image", BlitzenCore::GCRenderingResourceSystemName);
			return false;
		}

		FILE* file = scopedFILE.m_pHandle;

		context.mBlockSize = GetDDSBlockSize(context.mDDSHeader, context.mDDSHeader10);
		size_t imageSize = GetDDSImageSizeBC(context.mDDSHeader.dwWidth, context.mDDSHeader.dwHeight, context.mDDSHeader.dwMipMapCount, (uint32_t)context.mBlockSize);

		size_t readSize = fread(pData, 1, imageSize, file);

		if (!pData)
		{
			BLIT_ERROR("%s: Failed to read texture data", BlitzenCore::GCRenderingResourceSystemName);
			return CE_LOAD_DDS_IMAGE_DATA_ERROR_CODE;
		}
		if (readSize != imageSize)
		{
			BLIT_ERROR("%s: Failed to read the correct amount of texture data. Expected: %u, Read: %u", BlitzenCore::GCRenderingResourceSystemName, imageSize, readSize);
			return CE_LOAD_DDS_IMAGE_DATA_ERROR_CODE;
		}

		// Success
		return imageSize;
	}

	bool TextureManager::AddTexture(const char* textureName, const char* originalPath)
	{
		BlitCL::FatString filepath{ strlen(BLITZEN_CLIENT_RPFMESH_DIRECTORY) + strlen(GCRpfTextureSubfolder) + strlen("/")};
		filepath.Format("%s%s/%s", BLITZEN_CLIENT_RPFMESH_DIRECTORY, GCRpfTextureSubfolder, textureName);

		constexpr bool LCFailIfExistsFlag = true;
		if (!BlitzenPlatform::PlatformCopyFile(originalPath, filepath.Get(), LCFailIfExistsFlag))
		{
			BLIT_ERROR("%s: Failed to copy dds file over to project folder", BlitzenCore::GCRenderingResourceSystemName);
			return false;
		}

		return true;
	}

	void TextureManager::ALLOC(uint32_t textureCount)
	{
		mTextureNames = reinterpret_cast<BlitCL::FatString*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Texture, textureCount * sizeof(BlitCL::FatString)));
		mAllocatedCount = textureCount;
	}

	TextureManager::~TextureManager()
	{
		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Texture, mTextureNames, mAllocatedCount * sizeof(BlitCL::FatString));
	}
}