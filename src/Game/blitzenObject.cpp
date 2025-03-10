#include "blitObject.h"

namespace BlitzenEngine
{
    void GameObject::Update()
	{
		BLIT_WARN("Update function not implemented, if this is intended, change the bDynamic boolean to 0")
		// This is a placeholder
	}

	GameObject::GameObject(const char* meshName /*="undefined"*/, uint8_t bDynamic /*=0*/) :
		m_meshIndex{ 0/*pResources->meshMap.Get(meshName).transformIndex*/ },
		m_bDynamic{ bDynamic }, m_transformIndex{ 0 }
	{
		BLIT_INFO("GAME OBJECT CREATED")
	}
}