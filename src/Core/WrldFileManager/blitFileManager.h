#include "Core/blitzenEngine.h"

namespace BlitzenCore
{
	constexpr const char* GCBlitzenWorldFileExtension = ".bwrld";
	constexpr const char* GCBlitzenWorldFileVersionHeader = "blitzen_world_file_ver->";
	constexpr const char* GCBlitWRLDFileVersion = "0.0.1";

	BLIT_OFFLINE_FUNC bool StartNewWRLDFile();

	bool ReadWRLDFile();
}