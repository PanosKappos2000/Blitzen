#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "Platform/Common/blitMappedFile.h"

namespace BlitzenEngine
{
	constexpr uint32_t GCMaxLoadedMaterialCount = 10'000;
	constexpr uint32_t GCMaxTextureCountPerMaterial = 4;

	enum class MaterialAlphaMode
	{
		Opaque, Transparent
	};

	enum class MaterialTextureIndices : uint32_t
	{
		AbledoIndex = 0,
		NormalIndex = 1,
		SpecularIndex = 2,
		EmissiveIndex = 3
	};

	struct MaterialData
	{
		MaterialAlphaMode transparencyFlag;
	};

	class MaterialManager
	{
	public:
		uint32_t* mMaterialTextureOffsets = nullptr;
		uint32_t mOffsetCount;
		uint32_t mAllocatedOffsetCount;

		MaterialData* mMatData = nullptr;
		uint32_t mDataCount = 0;
		uint32_t mAllocatedDataCount = 0;

		BLIT_OFFLINE_FUNC bool AddMaterialTextureOffsets(uint32_t offset);

		BLIT_OFFLINE_FUNC bool AddMaterialData(MaterialAlphaMode transparencyFlag);

		// Allocates data arrays for materials. Count should not include sizeof(Material). It's done inside the function
		void ALLOC(uint32_t materialCount);

		~MaterialManager();

		void ResetContext();

		BLIT_OFFLINE_FUNC bool DefineMaterial(BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE& textureNamesBinstrFile, const char* albedoTextureName, const char* normalTextureName,
			const char* specularTextureName, const char* emissiveTextureName);
	};
}