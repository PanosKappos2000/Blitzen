#if defined(_WIN32)
#include "blitWASAPIResources.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenWASAPI
{
	bool CreateMixFormat(IAudioClient* audioClient, WAVEFORMATEX* pFormat, UINT32& outBufferFrames)
	{
		HRESULT mixRes;

		mixRes = audioClient->GetMixFormat(&pFormat);
		if (FAILED(mixRes))
		{
			BLIT_ERROR("%s: Failed to get mix format.", BlitzenCore::GCWasapiBackendSystemName);
			BlitzenPlatform::LogWin32HresultError(mixRes);
			return false;
		}

		mixRes = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED,
			0,                        // Flags (e.g., loopback or event-driven)
			0,                        // Desired buffer duration (0 = use default)
			0,                        // Periodicity (0 = default)
			pFormat,                  // Format
			nullptr                   // Audio session GUID (null = default)
		);
		if (FAILED(mixRes))
		{
			BLIT_ERROR("%s: Failed to initialize audio client.", BlitzenCore::GCWasapiBackendSystemName);
			BlitzenPlatform::LogWin32HresultError(mixRes);
			return false;
		}

		UINT32 bufferFrames = 0;
		mixRes = audioClient->GetBufferSize(&bufferFrames);
		if (FAILED(mixRes))
		{
			BLIT_ERROR("%s: Failed to get buffer size.", BlitzenCore::GCWasapiBackendSystemName);
			BlitzenPlatform::LogWin32HresultError(mixRes);
			return false;
		}

		outBufferFrames = bufferFrames;

		return true;
	}

	AUDIO_FORMAT::~AUDIO_FORMAT()
	{
		if(mHandle != nullptr) CoTaskMemFree(mHandle);
	}
}

#endif