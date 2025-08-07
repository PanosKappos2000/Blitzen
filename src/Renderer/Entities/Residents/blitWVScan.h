#include "blitResidentManager.h"
#include "Core/BlitzenWorld/blitzenUserInterface.h"

namespace BlitzenEngine
{
	static void RotatingKittenFunc(BlitzenEngine::Resident resident, float deltaTime)
	{
		constexpr float movementSpeed = 1.f;
		RotateResidentYaw(resident, movementSpeed, deltaTime);
		RotateResidentPitch(resident, movementSpeed, deltaTime);
	}

	void WorldTickLogic(WORLD_RESIDENTS* residents, float deltaTime)
	{
		for (Resident wv = 0; wv < residents->mWorldVariableCount; ++wv)
		{
			WorldVariableType wvType = residents->mWorldVariableTypes[wv].typeID;
			switch (wvType)
			{
			case 1: RotatingKittenFunc(wv, deltaTime);  break;
			default: break;
			}
		}
	}
}