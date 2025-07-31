#include "blitMaterial.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/blitMemory.h"

namespace BlitzenEngine
{
	bool MaterialManager::AddMaterial(MaterialAlphaMode alphaMode, const Material& matIndices)
	{
#if defined(BLIT_OFFLINE_FUNC)
		if (mMaterialCount >= GCMaxLoadedMaterialCount)
		{
			BLIT_ERROR("%s: Cannot allocate any more materials for current context", BlitzenCore::GCRenderingResourceSystemName);
			return false;
		}

		if (mMaterialCount >= mAllocatedCount)
		{
			uint32_t newCount = mAllocatedCount * 2 < GCMaxLoadedMaterialCount ? mMaterialCount * 2 : mAllocatedCount;
			BlitzenCore::BlitReAdjustMemoryAllocation(mMaterials, newCount * sizeof(Material), mAllocatedCount * sizeof(Material), BlitzenCore::AllocationType::Material);
			mAllocatedCount = newCount;
		}

		BlitzenPlatform::ConstMemCopy(&mMaterials[mMaterialCount], &matIndices, sizeof(Material));

		auto& matData = mMatData[mMaterialCount];
		matData.transparencyFlag = alphaMode;
		
		mMaterialCount++;
		mNotLoadedCount++;
#endif
		return true;
	}

	void MaterialManager::ALLOC(uint32_t materialCount)
	{
		mMaterials = reinterpret_cast<Material*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Material, materialCount * sizeof(Material)));
		mAllocatedCount = materialCount;

#if defined(BLIT_OFFLINE_BUILD)
		mMatData = reinterpret_cast<MaterialData*>(BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::Material, materialCount * sizeof(MaterialData)));
#endif
	}

	MaterialManager::~MaterialManager()
	{
		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Material, mMaterials, mAllocatedCount * sizeof(Material));

#if defined(BLIT_OFFLINE_BUILD)

		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::Material, mMatData, mAllocatedCount * sizeof(MaterialData));
#endif
	}
}