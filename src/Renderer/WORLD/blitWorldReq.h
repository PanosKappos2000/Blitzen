#include "Core/blitzenEngine.h"

namespace BlitzenWorld
{
	uint32_t GetCurrentWorldVariableCount();

	// Returns true static collider count + max world variable collider count. Useful for allocating array based on all colliders
	uint32_t GetCurrentColliderCount();

	// Return dynamic(WV) collider count
	uint32_t GetCurrentWorldVariableColliderCount();

	uint32_t GetStaticColliderCount();

	uint32_t GetCurrentTransformCount();

	uint32_t GetStaticTransformCount();
}