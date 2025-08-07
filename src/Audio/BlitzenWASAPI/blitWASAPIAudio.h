#if defined(_WIN32)
#pragma once
#include "Core/blitzenEngine.h"
#include "Platform/blitPlatformContext.h"
#include <mmdeviceapi.h>
#include <audioclient.h>        
#include <functiondiscoverykeys_devpkey.h> 
#include <wrl/client.h>

namespace BlitzenWASAPI
{
	class AUDIO_FORMAT
	{
	public:
		WAVEFORMATEX* mHandle = nullptr;

		~AUDIO_FORMAT();
	};

	template<class WASAPITYPE>
	using WASAPIWRAPPER = Microsoft::WRL::ComPtr<WASAPITYPE>;

	class AudioEngine
	{
	public:
		WASAPIWRAPPER<IMMDevice> mDevice{ nullptr };
		WASAPIWRAPPER<IAudioClient> mAudioClient{ nullptr };
		AUDIO_FORMAT mAudioFormat;
		UINT32 mBufferFrames = 0;
	};
}

#endif