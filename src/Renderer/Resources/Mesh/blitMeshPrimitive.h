#pragma once
#include "Renderer/Resources/blitShaderResources.h"

namespace BlitzenEngine
{
	
	constexpr uint32_t CE_MAX_INSTANCES_PER_LOD = 100'000;
	constexpr uint32_t CE_MAX_LOD_COUNT = BlitzenCore::Ce_MaxMeshPrimitivesCount * BlitzenCore::Ce_MaxLodCountPerSurface;

	struct MeshPrimitiveData
	{
		BlitzenCore::BIG_BOOL m_primitiveTransparencyFlags{ BlitzenCore::BB_FALSE };
		uint32_t m_primitiveVertexCount{ 0 };
		uint32_t m_primitiveVertexOffset{ UINT32_MAX };
	};

	struct MeshPrimitivesContainer
	{
		MeshPrimitiveData m_meshPrimitiveData[BlitzenCore::Ce_MaxMeshPrimitivesCount];
		PrimitiveSurface m_meshPrimitives[BlitzenCore::Ce_MaxMeshPrimitivesCount];
		uint32_t m_meshPrimitivesCount{ 0 };

		LodData m_LODs[CE_MAX_LOD_COUNT];
		uint32_t m_LODCount{ 0 };
	};
}