#pragma once
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	constexpr uint32_t CE_MAX_MESH_PRIMITIVE_VERTEX_COUNT = BlitzenCore::Ce_MaxWorldVertexCount;
	constexpr uint32_t CE_MAX_MESH_PRIMITIVE_INDEX_COUNT = BlitzenCore::Ce_MaxWorldVertexIndicesCount;

	class PrimitiveContainer
	{
	public:

		Vertex* m_vertices{ nullptr };
		uint32_t m_vertexCount{ 0 };
		uint32_t* m_indices{ nullptr };
		uint32_t m_vtxIdxCount{ 0 };

		uint32_t m_mapVtxCount{ 0 };
		uint32_t m_mapIdxCount{ 0 };

		VtxPos* m_vertexPositions{ nullptr };
		VtxTexCoords* m_vertexUVs{ nullptr };
		VtxNormals* m_vertexNormals{ nullptr };
		VtxTangents* m_vertexTangents{ nullptr };

		void ALLOC();
		void CLEAN();
		~PrimitiveContainer();

		bool AddVertices(Vertex* vertices, uint32_t count);
		bool AddIndices(uint32_t* indices, uint32_t count);

		bool OverrideVertices(Vertex* vertices, uint32_t count);
		bool OverrideIndices(uint32_t* indices, uint32_t count);
	};

	bool GenerateHlslVertices(PrimitiveContainer& context);

	struct HLSL_VTX_CONTEXT
	{
		VtxPos* m_vtxPosArr{ nullptr };
		VtxNormals* m_vtxNrmArr{ nullptr };
		VtxTangents* m_vtxTngArr{ nullptr };
		VtxTexCoords* m_texCoordArr{ nullptr };
	};
	void ConvertClassicVerticesToHlslFormat(HLSL_VTX_CONTEXT& hlslCtx, Vertex* classicVtxArr, uint32_t count);
}