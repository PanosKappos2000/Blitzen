#include "blitMaterial.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/blitMemory.h"
#include "Renderer/WORLD/blitWorldMap.h"

namespace BlitzenEngine
{
	void MaterialManager::ResetContext()
	{
		mDataCount = 0;
		mOffsetCount = 0;
	}

	bool MaterialManager::AddMaterialTextureOffsets(uint32_t materialTextureOffset)
	{
#if defined(BLIT_OFFLINE_FUNC)
		if (mOffsetCount >= GCMaxLoadedMaterialCount)
		{
			BLIT_ERROR("%s: Materials overflow", BlitzenCore::GCRenderingResourceSystemName);
			return false;
		}

		if (mOffsetCount >= mAllocatedOffsetCount)
		{
			uint32_t newCount = mAllocatedOffsetCount * 2 < GCMaxLoadedMaterialCount ? mAllocatedOffsetCount * 2 : GCMaxLoadedMaterialCount;
			BlitzenCore::BlitReAdjustMemoryAllocation(mMaterialTextureOffsets, newCount * sizeof(uint32_t), mAllocatedOffsetCount * sizeof(uint32_t), BlitzenCore::AllocationType::Material);
			mAllocatedOffsetCount = newCount;
		}

		mMaterialTextureOffsets[mOffsetCount] = materialTextureOffset;
		mOffsetCount++;
		
#endif
		return true;
	}

	bool MaterialManager::AddMaterialData(MaterialAlphaMode mode)
	{
		if (mDataCount >= GCMaxLoadedMaterialCount)
		{
			BLIT_ERROR("%s: Material data overflow", BlitzenCore::GCRenderingResourceSystemName);
			return false;
		}

		if (mDataCount >= mAllocatedDataCount)
		{
			uint32_t newCount = mAllocatedDataCount * 2 < GCMaxLoadedMaterialCount ? mAllocatedDataCount * 2 : GCMaxLoadedMaterialCount;
			BlitzenCore::BlitReAdjustMemoryAllocation(mMatData, newCount * sizeof(MaterialData), mAllocatedDataCount * sizeof(MaterialData), BlitzenCore::AllocationType::Material);
			mAllocatedDataCount = newCount;
		}

		mMatData[mDataCount].transparencyFlag = mode;

		mDataCount++;
		return true;
	}

	void MaterialManager::ALLOC(uint32_t materialCount)
	{
		mMaterialTextureOffsets = reinterpret_cast<uint32_t*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Material, materialCount * sizeof(Material)));
		mAllocatedOffsetCount = materialCount;

#if defined(BLIT_OFFLINE_BUILD)
		mMatData = reinterpret_cast<MaterialData*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Material, materialCount * sizeof(MaterialData)));
		mAllocatedDataCount = materialCount;
#endif
	}

	bool MaterialManager::DefineMaterial(BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE& textureNamesBinstrFile, const char* albedoTextureName, const char* normalTextureName,
		const char* specularTextureName, const char* emissiveTextureName)
	{
		//ReadStringDataFromBINSTRFile()
		return true;
	}

	MaterialManager::~MaterialManager()
	{
		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Material, mMaterialTextureOffsets, mAllocatedOffsetCount * sizeof(Material));

#if defined(BLIT_OFFLINE_BUILD)

		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Material, mMatData, mAllocatedDataCount * sizeof(MaterialData));
#endif
	}
}