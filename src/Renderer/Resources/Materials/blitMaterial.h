#pragma once
#include "Renderer/Resources/blitShaderResources.h"

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
		Material* mMaterials = nullptr;
		MaterialData* mMatData = nullptr;
		uint32_t mMaterialCount = 0;
		uint32_t mAllocatedCount = 0;
		uint32_t mNotLoadedCount = 0;

		BLIT_OFFLINE_FUNC bool AddMaterial(MaterialAlphaMode alphaMode, const Material& matIndices);

		// Allocates data arrays for materials. Count should not include sizeof(Material). It's done inside the function
		void ALLOC(uint32_t materialCount);

		~MaterialManager();
	};
}