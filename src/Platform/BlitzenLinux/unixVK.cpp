#if defined(linux)

#include "Platform/blitPlatformContext.h"

namespace BlitzenPlatform
{
	void UNSET_VALIDATION_LAYER_LENS()
	{
		//unsetenv("VK_INSTANCE_LAYERS");
	}
}

#endif