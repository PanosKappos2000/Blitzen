#if defined(_WIN32)

#pragma once

#include "Renderer/BlitzenDX12/dx12Renderer.h"
#include "Renderer/Entities/DynamicTransform/blitDynamicTransform.h"

namespace BlitzenDX12
{
	void WaitForFrame(Dx12Renderer* pContext);

	void Generate_HI_Z(Dx12Renderer* pContext);

	void PassDynamicTransformToRenderer(Dx12Renderer* pContext, BlitzenEngine::DynamicTransform* dynamicTransforms, uint32_t dynamicTransformCount, BlitzenEngine::MeshTransform* pTransform);
}

#endif