#include "blitWorldMap.h"
#include "Platform/Common/blitMappedFile.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
	LOAD_WRLD_MAP_RES LoadWORLDMapFromDisk(const char* filePath, WORLD_RESIDENTS* pWorldResidents)
	{
		return LOAD_WRLD_MAP_RES::SUCCESS;
	}

	UPLOAD_WRLD_MAP_RES UploadWORLDMapToDisk(const char* mapName, WORLD_RESIDENTS* pWorldResidents)
	{
		BlitCL::String mapFilepath{ GCClientWorldMapDirectory };
		mapFilepath.Append(const_cast<char*>(mapName));

		BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE worldMapFile;
		auto worldMapRes = worldMapFile.OpenWrite(mapFilepath.GetClassic(), sizeof(WORLD_RESIDENTS));
		if (BlitzenPlatform::CHECK_BLIT_MMF_RES_FOR_ERROR(worldMapRes))
		{
			return UPLOAD_WRLD_MAP_RES::ERROR_OPENING_FILE;
		}

		return UPLOAD_WRLD_MAP_RES::SUCCESS;
	}
}