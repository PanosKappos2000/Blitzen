#include "Core/blitzenEngine.h"
#include "Renderer/Entities/Residents/blitResidentManager.h"

namespace BlitzenEngine
{
	enum class LOAD_WRLD_MAP_RES : int64_t
	{
		SUCCESS = BlitzenCore::CE_BLITZEN_SUCCESS,
		FATAL = BlitzenCore::CE_BLITZEN_FATAL,
	};
	LOAD_WRLD_MAP_RES LoadWORLDMapFromDisk(const char* filePath, WORLD_RESIDENTS* pWorldResidents);

	enum class UPLOAD_WRLD_MAP_RES : int64_t
	{
		SUCCESS = BlitzenCore::CE_BLITZEN_SUCCESS,
		FATAL = BlitzenCore::CE_BLITZEN_FATAL,
	};
	UPLOAD_WRLD_MAP_RES UploadWORLDMapToDisk(const char* filepath, WORLD_RESIDENTS* pWorldResidents);

	BLIT_OFFLINE_FUNC bool UpdateWorldMapResources(const char* filepath);

	bool UpdateWorldMapResidents(const char* filepath);

	inline const char* LOAD_WRLD_MAP_RES_TO_STRING(LOAD_WRLD_MAP_RES res)
	{
		return "";
	}

	inline const char* UPLOAD_WRLD_MAP_RES_TO_STRING(UPLOAD_WRLD_MAP_RES res)
	{
		return "";
	}
}