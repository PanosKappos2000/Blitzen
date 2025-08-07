#pragma once
#include "BlitzenWASAPI/blitWASAPIAudio.h"

namespace BlitzenEngine
{

#if defined(WIN32)
	using AudioEngine = BlitzenWASAPI::AudioEngine;
	using AudioEnginePtrType = BlitzenWASAPI::AudioEngine*;
#elif defined(linux)
	using AudioEngine = BLIT_STRAIGHTHANDLE;
	using AudioEnginePtrType = BLIT_STRAIGHTHANDLE;
#else
	static_assert(false, "Platform not supported");
#endif

	bool AudioEngineInit(AudioEnginePtrType pAudio);
}