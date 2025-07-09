#include "Renderer/Resources/Terrain/blitTerrain.h"
#include "BlitCL/blitDynamicArr.h"
#include "BlitzenMathLibrary/blitML.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/DbLog/blitLogger.h"
#include "Renderer/Resources/blitShaderShared.h"

namespace BlitGenerator
{
    // Generates a grid terrain with sine-wave bumps
    bool GenerateTerrainMesh(BlitzenEngine::TerrainContainer& container)
    {
        constexpr uint32_t GridSize = 64;//BLIT_COLLISION_GRID_EXTENT;                  // Number of vertices per row/column
        constexpr float CellSize = 1.f;     //BLIT_COLLISION_GRID_CELL_EXTENT;          // Distance between vertices
        constexpr float Amplitude = 3.0f;                                               // Height of the bumps
        constexpr float Frequency = 0.2f;                                               // Frequency of sine waves

        BlitCL::DynamicArray<BlitzenEngine::VtxPos> terrainVertices{ GridSize * GridSize };
        BlitCL::DynamicArray<uint32_t> terrainIndices;

        uint32_t vtxId = 0;
        // Vertex generation
        for (uint32_t z = 0; z < GridSize; ++z)
        {
            for (uint32_t x = 0; x < GridSize; ++x)
            {
                BLIT_ASSERT(vtxId < terrainVertices.GetSize());

                float posX = x * CellSize;
                float posZ = z * CellSize;

                // Sine wave for height (replace with noise later)
                float height = BlitML::Sin(posX * Frequency) * BlitML::Cos(posZ * Frequency) * Amplitude;

                terrainVertices[vtxId] = BlitML::vec3{ posX, height, posZ };
                vtxId++;
            }
        }

        // Index generation (two triangles per quad)
        for (uint32_t z = 0; z < GridSize - 1; ++z)
        {
            for (uint32_t x = 0; x < GridSize - 1; ++x)
            {
                uint32_t topLeft = z * GridSize + x;
                uint32_t topRight = topLeft + 1;
                uint32_t bottomLeft = (z + 1) * GridSize + x;
                uint32_t bottomRight = bottomLeft + 1;

                // Triangle 1
                terrainIndices.PushBack(topLeft);
                terrainIndices.PushBack(bottomLeft);
                terrainIndices.PushBack(topRight);

                // Triangle 2
                terrainIndices.PushBack(topRight);
                terrainIndices.PushBack(bottomLeft);
                terrainIndices.PushBack(bottomRight);
            }
        }

        if (!container.AppendVertices(terrainVertices.Data(), uint32_t(terrainVertices.GetSize())))
        {
            BLIT_ERROR("%s: Failed to copy new terrain vertices to general terrain vertices array", BlitzenCore::CE_RESOURCE_SYSTEM_NAME);
            return false;
        }

        if (!container.AppendIndices(terrainIndices.Data(), uint32_t(terrainIndices.GetSize())))
        {
            BLIT_ERROR("%s: Failed to copy new terrain indices to general terrain indices array", BlitzenCore::CE_RESOURCE_SYSTEM_NAME);
            return false;
        }

        return true;
    }
}