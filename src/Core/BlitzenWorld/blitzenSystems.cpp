#include "blitzenWorldPrivate.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenWorld
{
	void BlitzenSystemsInit(BLITZEN_SYSTEM_CONTEXT* pSYSTEM)
	{
		pSYSTEM->pDASHER->AllocRenderingLoadingContext(pSYSTEM->pWORLD->BMPR.Data());

#if defined(BLIT_OFFLINE_BUILD)
		if (!BlitzenEngine::DasherDefineEditor(pSYSTEM->pWORLD->BMPR.Data(), pSYSTEM->pDASHER))
		{
			BlitzenCore::FORCE_ASSERT_CORE_ISSUE(BlitzenCore::GCBlitzenSystemManager, BlitzenCore::GCDasherEditorSystemName, "FAILED ON STARTUP");
		}
#endif

		if (!BlitzenEngine::AudioEngineInit(pSYSTEM->pJingle))
		{
			BlitzenCore::FORCE_ASSERT_CORE_ISSUE(BlitzenCore::GCBlitzenSystemManager, BlitzenCore::GCJingleAudioSystemName, "FAILED ON STARTUP");
		}
	}
}