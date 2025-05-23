#include "blitTextures.h"

namespace BlitzenEngine
{
	bool TextureManager::AddTexture(const char* textureName)
	{
		if (m_textureCount >= BlitzenCore::Ce_MaxTextureCount)
		{
			BLIT_ERROR("Max Texture count exceeded");
			return false;
		}

		m_textureIDMap.Insert(textureName, m_textureCount);
		m_textureCount++;
		return true;
	}

	bool TextureManager::AddMaterial(uint32_t albedoId, uint32_t normalId, uint32_t specularId, uint32_t emissiveId, const char* name /*="BLIT_DO_NOT_ADD_TO_MATERIAL_MAP"*/)
	{
		if (m_materialCount >= BlitzenCore::Ce_MaxMaterialCount)
		{
			BLIT_ERROR("Max Material count exceeded");
			return false;
		}

		auto pMaterial = &m_materials[m_materialCount];

		pMaterial->albedoTag = albedoId;
		pMaterial->normalTag = normalId;
		pMaterial->specularTag = specularId;
		pMaterial->emissiveTag = emissiveId;

		pMaterial->materialId = m_materialCount;

		if (name != "BLIT_DO_NOT_ADD_TO_MATERIAL_MAP")
		{
			m_pMaterialTable.Insert(name, pMaterial);
		}

		m_materialCount++;
		return true;
	}

	uint8_t OpenDDSImageFile(const char* filepath, DDS_HEADER& header, DDS_HEADER_DXT10& header10, BlitzenPlatform::C_FILE_SCOPE& scopedFILE)
	{
		if (!scopedFILE.Open(filepath, BlitzenPlatform::FileModes::Read, 1))
		{
			return 0;
		}

		auto file = scopedFILE.m_pHandle;

		unsigned int magic = 0;

		if (fread(&magic, sizeof(magic), 1, file) != 1 || magic != FourCC("DDS "))
		{
			BLIT_ERROR("Invalid DDS signature in file: %s", filepath);
			return 0;
		}

		if (fread(&header, sizeof(header), 1, file) != 1)
		{
			BLIT_ERROR("Failed to read DDS header data from file: %s", filepath);
			return 0;
		}

		if (header.ddspf.dwFourCC == FourCC("DX10") && fread(&header10, sizeof(header10), 1, file) != 1)
		{
			BLIT_ERROR("Failed to readd DDS header10 data from file: %s", filepath);
			return 0;
		}

		if (header.dwSize != sizeof(header) || header.ddspf.dwSize != sizeof(header.ddspf))
		{
			BLIT_ERROR("Invalid DDS header size for file: %s", filepath);
			return 0;
		}

		// These could be removed later, they seem to be ignoring certain types of textures
		if (header.dwCaps2 & (BlitzenCore::DDSCAPS2_CUBEMAP | BlitzenCore::DDSCAPS2_VOLUME))
		{
			return 0;
		}
		if (header.ddspf.dwFourCC == FourCC("DX10") && header10.resourceDimension != BlitzenCore::DDS_DIMENSION_TEXTURE2D)
		{
			return 0;
		}

		return 1;
	}

	size_t GetDDSImageSizeBC(unsigned int width, unsigned int height, unsigned int levels, unsigned int blockSize)
	{
		size_t result = 0;
		for (unsigned int i = 0; i < levels; ++i)
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
			switch (header10.dxgiFormat)
			{
			case BlitzenEngine::DXGI_FORMAT_BC1_UNORM:
			case BlitzenEngine::DXGI_FORMAT_BC1_UNORM_SRGB:
			case BlitzenEngine::DXGI_FORMAT_BC4_UNORM:
			case BlitzenEngine::DXGI_FORMAT_BC4_SNORM:
			{
				return 8;
			}

			case BlitzenEngine::DXGI_FORMAT_BC2_UNORM:
			case BlitzenEngine::DXGI_FORMAT_BC2_UNORM_SRGB:
			case BlitzenEngine::DXGI_FORMAT_BC3_UNORM:
			case BlitzenEngine::DXGI_FORMAT_BC3_UNORM_SRGB:
			case BlitzenEngine::DXGI_FORMAT_BC5_UNORM:
			case BlitzenEngine::DXGI_FORMAT_BC5_SNORM:
			case BlitzenEngine::DXGI_FORMAT_BC6H_UF16:
			case BlitzenEngine::DXGI_FORMAT_BC6H_SF16:
			case BlitzenEngine::DXGI_FORMAT_BC7_UNORM:
			case BlitzenEngine::DXGI_FORMAT_BC7_UNORM_SRGB:
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
}