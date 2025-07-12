#include "Renderer/Resources/Mesh/blitTriangle.h"
#include "Core/DbLog/blitLogger.h"
#include "BlitCL/blitDynamicArr.h"
#include <float.h>

// IMPORTANT NOTE:
// ALL GENERATOR FILES CONTAIN SIMPLIFIED VERSIONS OF THE ALGORITHMS FOUND IN MESHOPTIMIZER LIBRARY BY ARSENY KAPOULKINE (zeux github)
// The recreation of these algorithms (instead of using meshoptimizer directly) is done for academic reasons and for better control over errors
// https://github.com/zeux/meshoptimizer

namespace BlitGenerator
{
	// Tuned to minimize the ACMR of a GPU that has a cache profile similar to NVidia and AMD
	inline constexpr VertexScoreTable CE_VERTEX_SCORE_TABLE =
	{
		{0.f, 0.779f, 0.791f, 0.789f, 0.981f, 0.843f, 0.726f, 0.847f, 0.882f, 0.867f, 0.799f, 0.642f, 0.613f, 0.600f, 0.568f, 0.372f, 0.234f},
		{0.f, 0.995f, 0.713f, 0.450f, 0.404f, 0.059f, 0.005f, 0.147f, 0.006f},
	};
	static_assert(BLIT_ARRAY_SIZE(CE_VERTEX_SCORE_TABLE.CACHE) == CE_CACHE_SIZE_MAX + 1);
	static_assert(BLIT_ARRAY_SIZE(CE_VERTEX_SCORE_TABLE.LIVE) == CE_VALENCE_MAX + 1);

	constexpr float CE_GET_VERTEX_SCORE_ERROR_CODE_PSEUDO_FLOAT = FLT_MAX;

	bool OptimizeVertexIndices(VTXIDX_OPTIMIZATION_CONTEXT& context)
	{
		if (context.m_idxCount % 3 != 0)
		{
			BLIT_ERROR("%s: Index count was not a multiple of 3", BlitzenCore::CE_MESH_DATA_GENERATOR_SYSTEM_NAME);
			return false;
		}

		if (context.m_idxCount == 0 || context.m_vtxCount == 0)
		{
			BLIT_ERROR("%s: Cannot work with vertex or index count of zero", BlitzenCore::CE_MESH_DATA_GENERATOR_SYSTEM_NAME);
			return false;
		}

		BlitCL::DynamicArray<uint32_t> indicesCopy{ context.m_idxCount };
		BlitzenCore::MANUAL_COPY(indicesCopy.Data(), context.m_idxArr, context.m_idxCount * sizeof(uint32_t));
		context.m_idxArr = indicesCopy.Data();

		constexpr uint32_t CacheSize = 16;
		uint32_t faceCount = context.m_idxCount / 3;

		BlitCL::DynamicArray<uint32_t> vertexOffsets{ context.m_vtxCount };
		BlitCL::DynamicArray<uint32_t> vertexCounts{ context.m_vtxCount };
		BlitCL::DynamicArray<uint32_t> vertexIdxData{ context.m_idxCount };
		TRIANGLE_ADJACENCY_CONTEXT triangleAdjacency{};
		triangleAdjacency.m_vertexCountersArr = vertexCounts.Data();
		triangleAdjacency.m_vertexOffsetsArr = vertexOffsets.Data();
		triangleAdjacency.m_triangleFaceData = vertexIdxData.Data();
		if (!BuildTriangleAdjacency(triangleAdjacency, context.m_idxArr, context.m_idxCount, context.m_vtxCount))
		{
			BLIT_ERROR("%s: Failed to build edje adacency", BlitzenCore::CE_MESH_DATA_GENERATOR_SYSTEM_NAME);
			return false;
		}

		// Live triangle counts; How many triangles remain that reference one vertex 
		// alias vertex counter as triangle will be removed after emitting them so the counts always match
		uint32_t* liveTrianglesArr = triangleAdjacency.m_vertexCountersArr;

		// Holds an emitted flag for each triangle. Confirms if a triangle has already been emitted into the optimized index buffer
		BlitCL::DynamicArray<BlitzenCore::FAT_BOOL> emittedFlags{ faceCount, BLIT_FAT_FALSE };

		// Vertices that are likely to be reused soon or almost done are better.
		// This lets the optimizer favor triangles that are : Nearby in cache, Cleaning up the vertex efficiently.	
		BlitCL::DynamicArray<float> vertexScores{ context.m_vtxCount, 0 };

		for (uint32_t vtx = 0; vtx < context.m_vtxCount; ++vtx)
		{
			float score = GetVertexScoreFromTable(-1, liveTrianglesArr[vtx]);
			if (score == CE_GET_VERTEX_SCORE_ERROR_CODE_PSEUDO_FLOAT)
			{
				BLIT_ERROR("%s: Failed to get vertex score from table", BlitzenCore::CE_MESH_DATA_GENERATOR_SYSTEM_NAME);
				return false;
			}

			vertexScores[vtx] = score;
		}

		// Calculates triangle scores
		// When deciding which triangle to emit next, it will pick the one with the highest score, because it is likely to:
		// Reuse existing cached vertices, Clear out some vertices that are almost done.
		BlitCL::DynamicArray<float> triangleScores{ faceCount };
		for (size_t i = 0; i < faceCount; ++i)
		{
			uint32_t triangleA = context.m_idxArr[i * 3 + 0];
			uint32_t triangleB = context.m_idxArr[i * 3 + 1];
			uint32_t triangleC = context.m_idxArr[i * 3 + 2];

			triangleScores[i] = vertexScores[triangleA] + vertexScores[triangleB] + vertexScores[triangleC];
		}

		// Needs two separate copies of the cache to avoid overwriting data during the update phase.
		// CE_CACHE_SIZE_MAX + 4 is a little bigger than the actual cache size, as a safety margin
		uint32_t cacheHolder [2 * (CE_CACHE_SIZE_MAX + 4)];
		uint32_t* cacheArr = cacheHolder;

		// a temporary storage used when simulating cache updates.
		uint32_t* cacheNew = cacheHolder + CE_CACHE_SIZE_MAX + 4;
		size_t cacheCount = 0;

		uint32_t currentTriangle = 0;
		uint32_t inputCursor = 1;

		uint32_t outputTriangle = 0;

		// Sentinel value keeping the loop going
		// When the current triangle gets a valid index the loop continues
		// It stops otherwise as it will have no bits set to one
		while (currentTriangle != ~0u)
		{
			if (outputTriangle >= faceCount)
			{
				BLIT_ERROR("%s: Generated triangle outside of the actual face count");
				return false;
			}

			// Saves the indices of the current triangle
			uint32_t triangleA = context.m_idxArr[currentTriangle * 3 + 0];
			uint32_t triangleB = context.m_idxArr[currentTriangle * 3 + 1];
			uint32_t triangleC = context.m_idxArr[currentTriangle * 3 + 2];

			// The optimized index buffer receives the three vertex indices of the triangle that is being emitted.
			// It gets stored at the location corresponding to how many triangles have already been written(outputTriangle * 3).
			// This is creating the output triangle list, in an optimized order.
			// Eg. If the original index buffer was unordered: [12, 3, 7, 5, 4, 6, 8, 1, 2] - The optimized result might reorder triangles like : [5, 4, 6, 8, 1, 2, 12, 3, 7]
			context.m_destinationArr[outputTriangle * 3 + 0] = triangleA;
			context.m_destinationArr[outputTriangle * 3 + 1] = triangleB;
			context.m_destinationArr[outputTriangle * 3 + 2] = triangleC;
			outputTriangle++;

			// The triangle is tagged as emitted and its score is set to 0
			// This triangle is no longer considered a candidate
			emittedFlags[currentTriangle] = BLIT_FAT_TRUE;
			triangleScores[currentTriangle] = 0;

			// Every time that a triangle is emitted, its three vertices are "hot".
			// So, they are placed at the front of the simulated vertex cache, because the next triangles might reuse them.
			// The new cache starts with the hot vertices and will have the rest of the vertices later
			size_t cacheWrite = 0;
			cacheNew[cacheWrite++] = triangleA;
			cacheNew[cacheWrite++] = triangleB;
			cacheNew[cacheWrite++] = triangleC;

			// old triangles
			for (size_t i = 0; i < cacheCount; ++i)
			{
				uint32_t IDX = cacheArr[i];
				// Puts IDX after current triangle vertices (cache write was incremented three times earlier)
				cacheNew[cacheWrite] = IDX;
				// Only increments cacheWrite if IDX is NOT one of the triangle's vertices
				cacheWrite += (IDX != triangleA) & (IDX != triangleB) & (IDX != triangleC);
			}

			// CacheArr becomes the current cache state, reflecting what you just built.
			// CacheNew becomes the temporary cache for the next triangle.
			uint32_t* cacheTemp = cacheArr;
			cacheArr = cacheNew;
			cacheNew = cacheTemp;
			// Cache limit force
			cacheCount = cacheWrite > CacheSize ? CacheSize : cacheWrite;

			// Goes throught the 3 vertices of the emitted triangle
			// For each vertex of the emitted triangle, removes that triangle from the vertex's adjacency list.
			// This reduces the size of the adjacency lists for future lookups, making future iterations faster
			// It also decrements the live triangle count for that vertex, meaning fewer triangles remain that reference this vertex.
			// REMINDER: This should only be called on first load of a resources, but it's still good to optimize
			for (uint32_t k = 0; k < 3; ++k)
			{
				// True index from index buffer
				uint32_t IDX = context.m_idxArr[currentTriangle * 3 + k];

				// Points to the start of the adjacency list for vertex IDX 
				uint32_t* neighbors = &triangleAdjacency.m_triangleFaceData[0] + triangleAdjacency.m_vertexOffsetsArr[IDX];

				// How many triangles still reference this vertex
				uint32_t neighborCount = triangleAdjacency.m_vertexCountersArr[IDX];

				// Goes through relevant vertrices. Looks for the triangle that was just emitted
				for (size_t i = 0; i < neighborCount; ++i)
				{
					uint32_t triangleID = neighbors[i];

					// Found ?
					if (triangleID == currentTriangle)
					{
						// Replaces with last element in list and decrements the counter
						neighbors[i] = neighbors[neighborCount - 1];
						triangleAdjacency.m_vertexCountersArr[IDX]--;
						break;// Done(each triangle appears once)
					}
				}
			}

			uint32_t bestTriangle = ~0u;
			float bestScore = 0;

			// Optimization Loop.
			// Updates the cache position for each vertex in cacheArr And For each of those vertices :
			// Recomputes the vertex score.
			// Visits all un-emitted triangles that use it(from adjacency).
			// Recomputes those triangles’ scores.
			// Tracks which one has the best score.
			// That best triangle becomes currentTriangle for the next iteration.
			for (size_t i = 0; i < cacheWrite; ++i)
			{
				uint32_t IDX = cacheArr[i];

				// No triangles are using this vertex
				if (triangleAdjacency.m_vertexCountersArr[IDX] == 0)
				{
					continue;
				}

				// Simulating a real GPU post-transform vertex cache.
				// If the vertex is within the last CacheSize entries(i.e., it would be in cache), it gets a cache position score.
				// If it's older (i.e., outside the window), it gets -1, which results in a cold cache penalty.
				int32_t cache_position = i >= CacheSize ? -1 : int32_t(i);

				// Gets the new score for this vertex, based on:
				// How recently it was used(cache_position),
				// How many triangles still use it(liveTriangleCount).
				// Keeps high-valence vertices (shared by many triangles) active,
				// Emitts triangles that reuse recent vertices.
				float score = GetVertexScoreFromTable(cache_position, liveTrianglesArr[IDX]);
				if (score == CE_GET_VERTEX_SCORE_ERROR_CODE_PSEUDO_FLOAT)
				{
					BLIT_ERROR("%s: Failed to get valid vertex score from table", BlitzenCore::CE_MESH_DATA_GENERATOR_SYSTEM_NAME);
					return false;
				}
				float scoreDiff = score - vertexScores[IDX];

				// Commits the new score computed earlier for vertex IDX.
				// This score will now reflect the current cache position and valence (remaining triangle references)
				vertexScores[IDX] = score;

				// Retrieves the list of triangles that use vertex IDX.
				// m_triangleFaceData : A flat array of triangle indices.
				// Offsets and counters map each vertex to its list of incident triangles(same layout as meshopt).
				// Purpose : These are the triangles affected by the score change of this vertex — because they use it.
				const uint32_t* neighborsBegin = &triangleAdjacency.m_triangleFaceData[0] + triangleAdjacency.m_vertexOffsetsArr[IDX];
				const uint32_t* neighborsEnd = neighborsBegin + triangleAdjacency.m_vertexCountersArr[IDX];

				// Loop over all triangles that reference this vertex.
				for (const uint32_t* it = neighborsBegin; it != neighborsEnd; ++it)
				{
					uint32_t triangleIDX = *it;

					// Should not go over a triangle that has already been emitted. Something must have gone wrong earlier
					if (emittedFlags[triangleIDX] != BLIT_FAT_FALSE)
					{
						BLIT_ERROR("%s: Found already emitted triangle while trying to optimized vertex indices", BlitzenCore::CE_MESH_DATA_GENERATOR_SYSTEM_NAME);
						return false;
					}

					// Updates this triangle’s total score based on the new vertex score.
					// Each triangle’s score is the sum of the scores of its three vertices.
					float triangleScore = triangleScores[triangleIDX] + scoreDiff;

					// This should not happen
					if (triangleScore <= 0)
					{
						BLIT_ERROR("%s: Cannot accept triangle score form zero and below for optimized vertex cache", BlitzenCore::CE_MESH_DATA_GENERATOR_SYSTEM_NAME);
						return false;
					}

					// Keeps track of the best triangle to emit next, based on the highest score.
					// This happens across all vertices in the current cache — not just one.
					// This is done for every vertex in the cache(from the earlier loop), So it naturally converges to a triangle that has the best cache reuse profile among all reachable candidates.
					bestTriangle = bestScore < triangleScore ? triangleIDX : bestTriangle;
					bestScore = bestScore < triangleScore ? triangleScore : bestScore;
					triangleScores[triangleIDX] = triangleScore;
				}
			}

			// Steps through input triangles in order if it hits a dead-end
			currentTriangle = bestTriangle;

			// If the optimization loop couldn’t find any more adjacent triangles worth emitting (all neighbors emitted), it calls a fallback.
			// This fallback linearly scans through all triangles(starting from a cursor), finds the next un-emitted triangle, and continues the loop.
			if (currentTriangle == ~0u)
			{
				currentTriangle = GetNextTriangleDeadEnd(inputCursor, &emittedFlags[0], faceCount);
			}
		}

		return true;
	}

	bool BuildTriangleAdjacency(TRIANGLE_ADJACENCY_CONTEXT& adjacency, uint32_t* indicesArr, uint32_t indexCount, uint32_t vertexCount)
	{
		uint32_t faceCount = indexCount / 3;

		// Templated
		BlitzenCore::BlitMemSet(adjacency.m_vertexCountersArr, 0, vertexCount);

		// For every time a vertex is met, its counter will be incremented
		for (size_t idx = 0; idx < indexCount; ++idx)
		{
			if (indicesArr[idx] >= vertexCount)
			{
				BLIT_ERROR("%s: Found index above vertex count while trying to build triangle adjacency", BlitzenCore::CE_MESH_DATA_GENERATOR_SYSTEM_NAME);
				return false;
			}

			adjacency.m_vertexCountersArr[indicesArr[idx]]++;
		}

		// offset table
		uint32_t offset = 0;
		// Places vertex offsets at the previous count in order
		for (size_t idx = 0; idx < vertexCount; ++idx)
		{
			adjacency.m_vertexOffsetsArr[idx] = offset;
			offset += adjacency.m_vertexCountersArr[idx];
		}

		if (offset != indexCount)
		{
			BLIT_ERROR("%s: Something went wrong while building triangle adjacency. The final vertex offset should be equal to index count");
			return false;
		}

		// For each triangle, adds its ID to the adjacency list of its three vertices
		for (uint32_t tr = 0; tr < faceCount; ++tr)
		{
			uint32_t triangleA = indicesArr[tr * 3 + 0];
			uint32_t triangleB = indicesArr[tr * 3 + 1]; 
			uint32_t triangleC = indicesArr[tr * 3 + 2];

			// Saves the triangle ID into the current offset for each vertex.
			// Also increments the offset for the next triangle using this vertex
			adjacency.m_triangleFaceData[adjacency.m_vertexOffsetsArr[triangleA]++] = tr;
			adjacency.m_triangleFaceData[adjacency.m_vertexOffsetsArr[triangleB]++] = tr;
			adjacency.m_triangleFaceData[adjacency.m_vertexOffsetsArr[triangleC]++] = tr;
		}

		// The previous loop incremented the offsets, but they need to point to the start of each vertex's data block again
		for (size_t i = 0; i < vertexCount; ++i)
		{
			if (adjacency.m_vertexOffsetsArr[i] < adjacency.m_vertexCountersArr[i])
			{
				BLIT_ERROR("%s: Something went wrong with the vertex offsets while building triangle adjacency", BlitzenCore::CE_MESH_DATA_GENERATOR_SYSTEM_NAME);
				return false;
			}

			adjacency.m_vertexOffsetsArr[i] -= adjacency.m_vertexCountersArr[i];
		}

		return true;
	}

	float GetVertexScoreFromTable(int32_t cachePosition, uint32_t liveTriangles)
	{
		if (cachePosition < -1 && cachePosition < int32_t(CE_CACHE_SIZE_MAX))
		{
			return CE_GET_VERTEX_SCORE_ERROR_CODE_PSEUDO_FLOAT;
		}

		uint32_t liveTrianglesClamped = liveTriangles < CE_VALENCE_MAX ? liveTriangles : CE_VALENCE_MAX;

		return CE_VERTEX_SCORE_TABLE.CACHE[1 + cachePosition] + CE_VERTEX_SCORE_TABLE.LIVE[liveTrianglesClamped];
	}

	uint32_t GetNextTriangleDeadEnd(uint32_t& inputCursor, BlitzenCore::FAT_BOOL* emittedFlags, uint32_t faceCount)
	{
		// Scans forward from inputCursor, looking for the next triangle that has not been emitted yet.
		while (inputCursor < faceCount)
		{
			if (emittedFlags[inputCursor] == BLIT_FAT_FALSE)
			{
				return inputCursor;
			}

			++inputCursor;
		}

		return ~0u;
	}
}