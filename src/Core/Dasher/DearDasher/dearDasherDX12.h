#if defined(_WIN32)

#pragma once
#include "dearDasherData.h"
#include "Renderer/BlitzenDX12/dx12Renderer.h"

namespace BlitzenIMGUI
{
	struct ImguiDX12
	{
		DEAR_DASHER_RETURN_CODE Init(BlitzenDX12::Dx12Renderer* pRenderer);

		BlitzenDX12::Dx12Renderer* m_pDX{ nullptr };
	};
}
#endif