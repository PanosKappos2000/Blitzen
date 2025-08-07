#if defined(_WIN32)
#pragma once
#include "blitWASAPIAudio.h"

namespace BlitzenWASAPI
{
	bool CreateMixFormat(IAudioClient* mAudioClient, WAVEFORMATEX* pFormat, UINT32& outBufferFrames);
}

#endif