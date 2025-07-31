#include "Core/blitMemory.h"

namespace BlitCL
{
#if defined(BLIT_OFFLINE_BUILD)
	class FlagMap
	{
	public:
		FlagMap(size_t dataSize) : mDataSize{ dataSize }
		{
			if (dataSize != 0)
			{
				mFlags = BlitzenCore::BlitAlloc<BlitzenCore::FAT_BOOL>(BlitzenCore::AllocationType::Hashmap, dataSize);
				BlitzenCore::BlitMemSet<BlitzenCore::FAT_BOOL>(mFlags, BLIT_FAT_FALSE, dataSize);
			}
		}

		~FlagMap()
		{
			if (mFlags != nullptr) BlitzenCore::BlitFree<BlitzenCore::FAT_BOOL>(BlitzenCore::AllocationType::Hashmap, mFlags, mDataSize);
		}

		FlagMap(const FlagMap&) = delete;
		FlagMap& operator=(const FlagMap&) = delete;

		inline void SwitchFlag(const char* hasher)
		{
			size_t id = HASH(hasher);

			if (id >= mDataSize) HandleOutOfBoundsHash(id);

			auto& flag = mFlags[id];
			flag = flag == BLIT_FAT_FALSE ? BLIT_FAT_TRUE : BLIT_FAT_FALSE;
		}

		inline bool Get(const char* hasher)
		{
			size_t id = HASH(hasher);
			if (id >= mDataSize) return false;
			return mFlags[id] != BLIT_FAT_FALSE;
		}

	private:
		BlitzenCore::FAT_BOOL* mFlags{ nullptr };
		size_t mDataSize;

		inline size_t HASH(const char*)
		{

		}

		inline void HandleOutOfBoundsHash(size_t outOfBounds)
		{
			BlitzenCore::FAT_BOOL* pTemp = mFlags;
			size_t newSize = outOfBounds + 1;

			// Allocates new block, copies the previous data over, sets new flags to false and gives the new block pointer to the member pointer / array
			BlitzenCore::FAT_BOOL* newBlock = BlitzenCore::BlitAlloc<BlitzenCore::FAT_BOOL>(BlitzenCore::AllocationType::Hashmap, newSize);
			BlitzenCore::BlitMemCopy(newBlock, mFlags, mDataSize * sizeof(BlitzenCore::FAT_BOOL));
			BlitzenCore::BlitMemSet<BlitzenCore::FAT_BOOL>(newBlock, BLIT_FAT_FALSE, newSize - mDataSize);
			mFlags = newBlock;
			
			// Frees previous memory block, given to pTemp at the start of the function. Gives the new size after and forgets about the old one
			BlitzenCore::BlitFree<BlitzenCore::FAT_BOOL>(BlitzenCore::AllocationType::Hashmap, pTemp, mDataSize);
			mDataSize = newSize;
		}
	};
#endif
}