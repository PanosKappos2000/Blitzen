#include "Core/blitzenEngine.h"
#include "Renderer/Resources/blitShaderShared.h"

namespace BMPR
{
#if defined(BLIT_DOUBLE_BUFFERING_PLUS)
	constexpr uint32_t GCMaxFramesInFlight = BLIT_FRAMES_IN_FLIGHT;
else
	constexpr uint32_t GCMaxFramesInFlight = 0;
#endif

#if defined(BLITZEN_CONFIGURATION_ENGINE_DEV)
	constexpr uint8_t GCGraphicsAPIDebugValidationFlag = 1;
#else
	constexpr uint8_t GCGraphicsAPIDebugValidationFlag = 0;
#endif

//----------------------------------------------------------------------------------------------------------
// PIPELINE STATE OBJECT ACCESSORS
//----------------------------------------------------------------------------------------------------------
    constexpr uint32_t GCPipelineStateObjectArraySize = 30;
    constexpr uint32_t GCPsoDrawCullStaticNoOccID = 0;
    constexpr uint32_t GCPsoDrawCullStaticResetID = 1;
    constexpr uint32_t GCPsoDrawCullStaticTemporalOccID = 2;
    constexpr uint32_t GCPsoDrawCullWorldVariableResetID = 3;
    constexpr uint32_t GCPsoDrawCullWorldVariableTemporalOccID = 4;
    constexpr uint32_t GCPsoDrawCullStaticInstancedResetID = 5;
    constexpr uint32_t GCPsoDrawCullStaticInstancedID = 6;
    constexpr uint32_t GCPsoDrawCullStaticDPOCCFirstPassID = 7;
    constexpr uint32_t GCPsoDrawCullStaticDPOCCSecondPassID = 8;
    constexpr uint32_t GCPsoClusterDrawCullStaticResetID = 9;
    constexpr uint32_t GCPsoClusterDrawCullStaticID = 10;
    constexpr uint32_t GCPsoClusterCullStaticCmdSetID = 11;
    constexpr uint32_t GCPsoClusterCullStaticID = 12;
    constexpr uint32_t GCPsoClusterCullBatchID = 13;
    constexpr uint32_t GCPsoHierarchicalZBufferMapID = 14;
    constexpr uint32_t GCPsoGridCellWorldVariableCountResetID = 15;
    constexpr uint32_t GCPsoGridCellColliderCountingID = 16;
    constexpr uint32_t GCPsoGridCellColliderIndicesOffsetID = 17;
    constexpr uint32_t GCPsoColliderIndicesSetID = 18;
    constexpr uint32_t GCPsoColliderTransformID = 19;
    constexpr uint32_t GCPsoCollisionResolveID = 20;
    constexpr uint32_t GCPsoColliderDebugCullResetID = 21;
    constexpr uint32_t GCPsoColliderDebugCullID = 22;

    constexpr uint32_t GCPsoColliderDebugDrawID = 23;
    constexpr uint32_t GCPsoGridCellsDebugDrawID = 24;
    constexpr uint32_t GCPsoBlitzenLoadScreenID = 25;
    constexpr uint32_t GCPsoStaticDrawID = 26;
    constexpr uint32_t GCPsoWorldVariableDrawID = 27;
    constexpr uint32_t GCPsoStaticInstancedDrawID = 28;
    constexpr uint32_t GCPsoStaticTransparentDrawID = 29;
    constexpr uint32_t GCPsoTerrainDrawID = 30;

}