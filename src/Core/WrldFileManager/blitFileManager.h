#pragma once
#include "Core/blitzenEngine.h"

namespace BlitzenCore
{
	constexpr const char* GCClientWorldFilepath= BLITZEN_CLIENT_WRLD_FILEPATH;
	constexpr const char* GCBlitzenWorldFileExtension = ".bwrld";
	constexpr const char* GCBlitzenWorldFileVersionHeader = "blitzen_world_file_ver->";
	constexpr const char* GCBlitWRLDFileVersion = "0.0.1";
	constexpr uint32_t GCBlitWRLDFileSize = 1024 * 1024;
	
	constexpr uint32_t GCWrldFileDataArrElementCount = 2;
	enum WrldFileHeaderIndices : uint32_t
	{
		WRLD_HEADER_INDICES_VERSION_ID = 0,
		WRLD_HEADER_INDICES_CURRENT_MAP_ID = 1,
	};
	struct WrldFileHeaderData
	{
		size_t dataOffset;
		size_t dataSize;
	};
	using WrldFileHeaderArr = WrldFileHeaderData[GCWrldFileDataArrElementCount];

	BLIT_OFFLINE_FUNC bool StartNewWRLDFile();

	bool ReadWRLDFile();

	bool UpdateWrldFile(const char* mapName);
}