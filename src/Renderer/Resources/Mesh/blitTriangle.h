#pragma once
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	struct PrimitiveContainer
	{
		PrimitiveContainer();
		~PrimitiveContainer();

		Vertex* m_vertices{ nullptr };
		uint32_t m_vertexCount{ 0 };
		uint32_t* m_indices{ nullptr };
		uint32_t m_vtxIdxCount{ 0 };

		bool AddVertices(Vertex* vertices, uint32_t count);
		bool AddIndices(uint32_t* indices, uint32_t count);
	};

	struct HLSL_PrimitiveContainer
	{
		HlslVtx* HLSL_VERTICES{nullptr};

		void ALLOC();

		~HLSL_PrimitiveContainer();
	};

}