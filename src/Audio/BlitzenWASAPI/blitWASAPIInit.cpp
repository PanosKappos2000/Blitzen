#if defined(_WIN32)
#include "Audio/blitAudio.h"
#include "Core/DbLog/blitLogger.h"
#include "blitWASAPIResources.h"

namespace BlitzenEngine
{
	bool AudioEngineInit(BlitzenWASAPI::AudioEngine* pAudio)
	{
        HRESULT initRes;

        initRes = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(initRes)) 
        {
            BLIT_ERROR("%s: Failed to initialize.", BlitzenCore::GCWasapiBackendSystemName);
            BlitzenPlatform::LogWin32HresultError(initRes);
            return false;
        }

        BlitzenWASAPI::WASAPIWRAPPER<IMMDeviceEnumerator> pEnumerator;
        initRes = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(pEnumerator.GetAddressOf()));
        if (FAILED(initRes)) 
        {
            BLIT_ERROR("%s: Failed to create instance.", BlitzenCore::GCWasapiBackendSystemName);
            BlitzenPlatform::LogWin32HresultError(initRes);
            return false;
        }

        initRes = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, pAudio->mDevice.GetAddressOf());
        if (FAILED(initRes))
        {
            BLIT_ERROR("%s: Failed to get default audio endPoint", BlitzenCore::GCWasapiBackendSystemName);
            BlitzenPlatform::LogWin32HresultError(initRes);
            return false;
        }

        initRes = pAudio->mDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(pAudio->mAudioClient.GetAddressOf()));
        if (FAILED(initRes))
        {
            BLIT_ERROR("%s: Failed to activate audio client", BlitzenCore::GCWasapiBackendSystemName);
            BlitzenPlatform::LogWin32HresultError(initRes);
            return false;
        }

        if (!BlitzenWASAPI::CreateMixFormat(pAudio->mAudioClient.Get(), pAudio->mAudioFormat.mHandle, pAudio->mBufferFrames)) return false;

		return true;
	}
}

#endif