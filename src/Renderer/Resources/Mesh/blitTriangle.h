#pragma once
#include "Renderer/Resources/blitShaderResources.h"
#include "BlitCL/DynamicArray.h"

namespace BlitzenEngine
{
	struct PrimitiveContainer
	{
		BlitCL::DynamicArray<Vertex> m_vertices{};
		BlitCL::DynamicArray<uint32_t> m_indices{};
		BlitCL::DynamicArray<Cluster> m_clusters{};
	};

	struct HLSL_PrimitiveContainer
	{
		BlitCL::DynamicArray<HlslVtx> HLSL_VERTICES{};
		BlitCL::DynamicArray<HCluster> HLSL_CLUSTERS{};
	};

}