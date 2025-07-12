#include "Renderer/Resources/Mesh/blitMeshPrimitive.h"
#include "BlitCL/blitDynamicArr.h"
#include <float.h>

// IMPORTANT NOTE:
// ALL GENERATOR FILES CONTAIN SIMPLIFIED VERSIONS OF THE ALGORITHMS FOUND IN MESHOPTIMIZER LIBRARY BY ARSENY KAPOULKINE (zeux github)
// The recreation of these algorithms (instead of using meshoptimizer directly) is done for academic reasons and for better control over errors
// https://github.com/zeux/meshoptimizer

namespace BlitGenerator
{
	float GetLODDegradationScale(BlitzenEngine::Vertex* vtxArr, uint32_t vertexCount)
	{
		// Initializes min and max values for the bounding box of the mesh, used to track the smallest and largest points on each axis (x, y, z).
		// Set to extreme values to ensure updates with actual vertex positions.
		float meshBoundingBoxMin[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
		float meshBoundingBoxMax[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

		// Loops through all vertices in the mesh and calculates the min and max positions for each axis (x, y, z) to define the bounding box that contains the mesh.
		for (uint32_t vtx = 0; vtx < vertexCount; ++vtx)
		{
			float* vtxAxisArr = &vtxArr[vtx].position.x;

			// Updates the min and max for each axis(x, y, z) to find the bounds of the mesh.
			for (uint32_t axis = 0; axis < 3; ++axis)
			{
				float vtxAxis = vtxAxisArr[axis];

				meshBoundingBoxMin[axis] = meshBoundingBoxMin[axis] > vtxAxis ? vtxAxis : meshBoundingBoxMin[axis];
				meshBoundingBoxMax[axis] = meshBoundingBoxMax[axis] < vtxAxis ? vtxAxis : meshBoundingBoxMax[axis];
			}
		}

		float extent = 0.f;

		// Finds the largest difference between the max and min values along each axis.
		// The extent is the largest size of the bounding box in world space.
		extent = (meshBoundingBoxMax[0] - meshBoundingBoxMin[0]) < extent ? extent : (meshBoundingBoxMax[0] - meshBoundingBoxMin[0]);
		extent = (meshBoundingBoxMax[1] - meshBoundingBoxMin[1]) < extent ? extent : (meshBoundingBoxMax[1] - meshBoundingBoxMin[1]);
		extent = (meshBoundingBoxMax[2] - meshBoundingBoxMin[2]) < extent ? extent : (meshBoundingBoxMax[2] - meshBoundingBoxMin[2]);

		return extent;
	}

	bool BuildEdgeAdjacency(EDGE_ADJACENCY_CONTEXT& adjacency, uint32_t* indicesArr, uint32_t indexCount, uint32_t vertexCount)
	{
		// Number of triangles
		uint32_t faceCount = indexCount / 3;

		// Holds adjacency offsets after the first element
		uint32_t* offsetsArr = &adjacency.m_offsetsArr[1];

		// Templated, does vertexCount * sizeof(dataType) inside
		BlitzenCore::BlitMemSet(offsetsArr, 0, vertexCount);

		// Iterates over the indices in the mesh's index array to count how many edges each vertex is connected to.
		for (size_t idx = 0; idx < indexCount; ++idx)
		{
			// Retrieves the corresponding vertex index
			uint32_t vtxIdx = indicesArr[idx];

			if (vtxIdx >= vertexCount)
			{
				return false;
			}

			// Increments the edge count for the corresponding vertex
			offsetsArr[vtxIdx]++;
		}

		// Starts filling offset table
		// This step calculates where the list of edges for each vertex begins in the adjacency list.
		uint32_t offset = 0;
		for (size_t idx = 0; idx < vertexCount; ++idx)
		{
			// Saves count
			uint32_t count = offsetsArr[idx];

			// Switches count to offset
			offsetsArr[idx] = offset;

			// Offset added to total
			offset += count;
		}

		// Offset should be equal to the index count by the ned of the loop, otherwise something went wrong.
		if (offset != indexCount)
		{
			return false;
		}

		// For each face (triangle), the adjacency data for the three vertices that form it needs to be updated.
		for (size_t i = 0; i < faceCount; ++i)
		{
			// Three vertex indices for each triangle
			uint32_t triangleA = indicesArr[i * 3 + 0];
			uint32_t triangleB = indicesArr[i * 3 + 1]; 
			uint32_t triangleC = indicesArr[i * 3 + 2];

			/* NOT SURE IF I WILL EVER BE USING A REMAP */
			//if (remap)
			//{
			//	a = remap[a];
			//	b = remap[b];
			//	c = remap[c];
			//}

			/**************************************************************************************************** 
			*	Updates the adjacency arrays for each vertex to store the next and previous edges				*
			*****************************************************************************************************/

			// Vertex A: Connected to B and C
			adjacency.m_edgesNextIndicesArr[offsetsArr[triangleA]] = triangleB;
			adjacency.m_edgesPreviousIndicesArr[offsetsArr[triangleA]] = triangleC;
			offsetsArr[triangleA]++;

			// Vertex B: Connected to C and A
			adjacency.m_edgesNextIndicesArr[offsetsArr[triangleB]] = triangleC;
			adjacency.m_edgesPreviousIndicesArr[offsetsArr[triangleB]] = triangleA;
			offsetsArr[triangleB]++;

			// Vertex C: Connected to A and B
			adjacency.m_edgesNextIndicesArr[offsetsArr[triangleC]] = triangleA;
			adjacency.m_edgesPreviousIndicesArr[offsetsArr[triangleC]] = triangleB;
			offsetsArr[triangleC]++;
		}

		// After filling the edge data, the offsets array is  finalized by setting the first entry to 0.
		// This is because the offsets are stored starting from index 1 in the adjacency structure.
		adjacency.m_offsetsArr[0] = 0;

		// Ensure the final offset for the last vertex equals the total number of indices in the mesh
		return adjacency.m_offsetsArr[vertexCount] == indexCount;
	}

	uint32_t DegradeLevelOfDetail(uint32_t* degradedIndices, uint32_t degradedIndexCount, LOD_DEGRADE_CONTEXT& context, float error, float* pError)
	{
		const uint32_t DEGRADATION_ERROR_CODE = UINT32_MAX;
		if (context.m_indexCount % 3 != 0)
		{
			return CE_DEGRADATION_ERROR_CODE;
		}
		if (degradedIndexCount > context.m_indexCount)
		{
			return CE_DEGRADATION_ERROR_CODE;
		}
		if (error < 0)
		{
			return CE_DEGRADATION_ERROR_CODE;
		}

		uint32_t nextLODCount{ 0 };

		// For the time being, I only pass the same array for previous and degraded indices. I will develop this logic if I ever need something different
		if (degradedIndices != context.m_previousIndices)
		{
			//BlitzenCore::BlitMemCopy(degradedIndices, context.m_previousIndices, )
		}

		BlitCL::DynamicArray<uint32_t> edgeAdjacencyOffsets{ context.m_vertexCount + 1 };
		BlitCL::DynamicArray<uint32_t> edgesPreviousIndices{ context.m_indexCount };
		BlitCL::DynamicArray<uint32_t> edgesNextIndices{ context.m_indexCount };
		EDGE_ADJACENCY_CONTEXT edgeAdjacencyContext{};
		edgeAdjacencyContext.m_offsetsArr = edgeAdjacencyOffsets.Data();
		edgeAdjacencyContext.m_edgesPreviousIndicesArr = edgesPreviousIndices.Data();
		edgeAdjacencyContext.m_edgesNextIndicesArr = edgesNextIndices.Data();
		if (!BuildEdgeAdjacency(edgeAdjacencyContext, degradedIndices, degradedIndexCount, context.m_vertexCount))
		{
			return CE_DEGRADATION_ERROR_CODE;
		}

		return nextLODCount;
	}
}