#pragma once
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	struct PrimitiveContainer
	{
		Vertex* m_vertices{ nullptr };
		uint32_t m_vertexCount{ 0 };
		uint32_t* m_indices{ nullptr };
		uint32_t m_vtxIdxCount{ 0 };

		VtxPos* m_vertexPositions{ nullptr };
		VtxTexCoords* m_vertexUVs{ nullptr };
		VtxNormals* m_vertexNormals{ nullptr };
		VtxTangents* m_vertexTangents{ nullptr };

		void ALLOC();
		~PrimitiveContainer();

		bool AddVertices(Vertex* vertices, uint32_t count);
		bool AddIndices(uint32_t* indices, uint32_t count);
	};

}