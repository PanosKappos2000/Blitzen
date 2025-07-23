#include "blitFileManager.h"
#include "BlitCL/blitString.h"
#include "Platform/Filesystem/blitCFILE.h"

namespace BlitzenCore
{
	bool StartNewWRLDFile()
	{
#if defined BLIT_OFFLINE_BUILD
		BlitCL::String pathToProject{ "../" };
		pathToProject.Append(const_cast<char*>(GCClientBlitProjectName));
		pathToProject.Append("/");
		pathToProject.Append(const_cast<char*>(GCClientBlitProjectName));
		pathToProject.Append(const_cast<char*>(GCBlitzenWorldFileExtension));

		if(BlitzenPlatform::FilepathExists(pathToProject.GetClassic()))
		{
			return false;
		}

		// Creates new file
		BlitzenPlatform::C_FILE_SCOPE scopedFile{};
		constexpr bool LCFileNotBinaryFlag = false;
		if (!scopedFile.Open(pathToProject.GetClassic(), BlitzenPlatform::FileModes::Write, LCFileNotBinaryFlag))
		{
			return false;
		}

		// Creates the header of the file, which will hold the version of the wrld files and define their behavior
		BlitCL::String firstLineHeader{GCBlitzenWorldFileVersionHeader};
		firstLineHeader.Append(const_cast<char*>(GCBlitWRLDFileVersion));
		if (!BlitzenPlatform::FilesystemWriteLine(scopedFile, firstLineHeader.GetClassic()))
		{
			return false;
		}

		// Creates the standard map data accessor
		BlitCL::String blitMinusMapData{ "blit_minus: " };
		blitMinusMapData.Append("test");
		if (!BlitzenPlatform::FilesystemWriteLine(scopedFile, blitMinusMapData.GetClassic()))
		{
			return false;
		}

		return true;
#endif
	}
}