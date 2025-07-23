#include "blitWorldMap.h"
#include "Platform/Common/blitMappedFile.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
	LOAD_WRLD_MAP_RES LoadWORLDMapFromDisk(const char* filePath, WORLD_RESIDENTS* pWorldResidents)
	{
		return LOAD_WRLD_MAP_RES::SUCCESS;
	}

	UPLOAD_WRLD_MAP_RES UploadWORLDMapToDisk(const char* filepath, WORLD_RESIDENTS* pWorldResidents)
	{
		return UPLOAD_WRLD_MAP_RES::SUCCESS;
	}
}