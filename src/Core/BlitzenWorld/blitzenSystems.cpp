#include "blitzenWorldPrivate.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenWorld
{
	void BlitzenSystemsInit(BLITZEN_SYSTEM_CONTEXT* pSYSTEM)
	{
		pSYSTEM->pDASHER->AllocRenderingLoadingContext(pSYSTEM->pWORLD->BMPR.Data());

#if defined(BLIT_OFFLINE_BUILD)
		BLIT_ASSERT(BlitzenEngine::DasherDefineEditor(pSYSTEM->pWORLD->BMPR.Data(), pSYSTEM->pDASHER));
#endif
	}
}