#include "blitFileManager.h"
#include "BlitCL/blitString.h"
#include "Platform/Filesystem/blitCFILE.h"
#include "Platform/Common/blitMappedFile.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenCore
{
	bool UpdateWrldFile(const char* mapName)
	{
		// Creates new file
		BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE scopedFile{};
		auto wrldOpenRes = scopedFile.OpenWrite(GCClientWorldFilepath, GCBlitWRLDFileSize);
		if (BlitzenPlatform::CHECK_BLIT_MMF_RES_FOR_ERROR(wrldOpenRes))
		{
			BLIT_ERROR("%s: Error while opening wrld for the first time. Got platform error: ", BlitzenCore::GCWRLDSystemName, BlitzenPlatform::GET_BLIT_MMF_RES_ERROR_STR(wrldOpenRes));
			return false;
		}

		// Creates the header of the file, which will hold the version of the wrld files and define their behavior
		WrldFileHeaderArr headerArr{};
		uint32_t offset = GCWrldFileDataArrElementCount * sizeof(WrldFileHeaderData);
		BlitCL::String firstLineHeader{ GCBlitzenWorldFileVersionHeader };
		firstLineHeader.Append(const_cast<char*>(GCBlitWRLDFileVersion));

		if (!BlitzenPlatform::WriteMemoryMappedFile(scopedFile, offset, firstLineHeader.GetSize(), firstLineHeader.Data()))
		{
			return false;
		}
		headerArr[WRLD_HEADER_INDICES_VERSION_ID].dataOffset = offset;
		headerArr[WRLD_HEADER_INDICES_VERSION_ID].dataSize = firstLineHeader.GetSize();
		offset += firstLineHeader.GetSize();

		char* currentMapName = "None";
		uint32_t currentMapSize = strlen(currentMapName);
		if (!BlitzenPlatform::WriteMemoryMappedFile(scopedFile, offset, currentMapSize, currentMapName))
		{
			return false;
		}
		headerArr[WRLD_HEADER_INDICES_CURRENT_MAP_ID].dataOffset = offset;
		headerArr[WRLD_HEADER_INDICES_CURRENT_MAP_ID].dataSize = currentMapSize;
		offset += currentMapSize;

		constexpr uint32_t LCStartOfFile = 0;
		if (!BlitzenPlatform::WriteMemoryMappedFile(scopedFile, LCStartOfFile, GCWrldFileDataArrElementCount * sizeof(WrldFileHeaderData), headerArr))
		{
			return false;
		}
	}

	bool StartNewWRLDFile()
	{
#if defined BLIT_OFFLINE_BUILD
		
		if(BlitzenPlatform::FilepathExists(GCClientWorldFilepath))
		{
			return false;
		}

		if (!UpdateWrldFile("None"))
		{
			return false;
		}

		if (!BlitzenPlatform::CreateDirectoryIfMissing(BLITZEN_CLIENT_WORLDMAPS_DIRECTORY))
		{
			return false;
		}

		if(!BlitzenPlatform::CreateDirectoryIfMissing(BLITZEN_CLIENT_RPFMESH_DIRECTORY))
		{
			return false;
		}

		return true;
#endif
	}

	bool ReadWRLDFile()
	{
		BlitCL::String pathToProject{ "../" };
		pathToProject.Append(const_cast<char*>(GCClientWorldFilepath));

		BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE scopedFile{};

		auto fileOpenRes = scopedFile.OpenRead(pathToProject.GetClassic());
		BLIT_ASSERT_MESSAGE(BLIT_CHECK_FATAL((int64_t)fileOpenRes), BlitzenPlatform::GET_BLIT_MMF_RES_ERROR_STR(fileOpenRes));

		if (BLIT_CHECK_FAIL((int64_t)fileOpenRes))
		{
			BLIT_ERROR("%s: Failed to open WRLD(client project) file: Received Platform Message: ", GCWRLDSystemName, BlitzenPlatform::GET_BLIT_MMF_RES_ERROR_STR(fileOpenRes));
			return false;
		}
	}
}