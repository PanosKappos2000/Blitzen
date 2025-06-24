#pragma once

#include "Renderer/Entities/Residents/blitResident.h"

namespace BlitzenEngine
{
	class WVRotatingKitten
	{
	public:
		void Start();

		void Tick();
	private:
		uint32_t residentID;
	};
}
