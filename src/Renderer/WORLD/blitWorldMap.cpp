#include "blitWorldMap.h"
#include "Platform/Common/blitMappedFile.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/DbLog/blitLogger.h"

namespace BlitzenEngine
{
	static const char* BuildWorldMapFilepath(BlitCL::String& container, const char* mapName)
	{
		if(container.GetSize() != 0)
		{
			BLIT_WARN("%s: The string container passed for World Map filepath was not empty", BlitzenCore::CE_WORLD_SYSTEM_NAME)
			container.Clear();
		}

		container.Append(const_cast<char*>(GCClientWorldMapDirectory));
		container.Append(const_cast<char*>(mapName));
		container.Append(const_cast<char*>(GCWorldMapFileExtension));
		return container.GetClassic();
	}

	static const char* BuildWorldMapResourceNamesFilepath(BlitCL::String& container, const char* mapName)
	{
		if (container.GetSize() != 0)
		{
			BLIT_WARN("%s: The string container passed for bmstr filepath was not empty", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			container.Clear();
		}

		container.Append(const_cast<char*>(GCClientWorldMapDirectory));
		container.Append(const_cast<char*>(mapName));
		container.Append(const_cast<char*>(GCWorldMapResourceNamesFileExtension));
		return container.GetClassic();
	}

	LOAD_WORLD_MAP_RES LoadWORLDMapFromDisk(const char* mapName, WORLD_RESIDENTS* pWorldResidents)
	{
		BlitCL::String mapContainer;
		const char* mapFilepath = BuildWorldMapFilepath(mapContainer, mapName);

		if (!BlitzenPlatform::FilepathExists(mapFilepath))
		{
			return LOAD_WORLD_MAP_RES::WORLD_MAP_FILE_NEVER_CREATED;
		}

		BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE worldMapFile;
		auto worldMapRes = worldMapFile.OpenRead(mapFilepath);
		if (BlitzenPlatform::CHECK_BLIT_MMF_RES_FOR_ERROR(worldMapRes))
		{
			return LOAD_WORLD_MAP_RES::ERROR_OPENING_FILE;
		}

		return LOAD_WORLD_MAP_RES::SUCCESS;
	}

	UPLOAD_WORLD_MAP_RES UploadWORLDMapToDisk(const char* mapName, WORLD_RESIDENTS* pWorldResidents)
	{
		BlitCL::String mapContainer;
		const char* mapFilepath = BuildWorldMapFilepath(mapContainer, mapName);

		size_t mapFileSize = GetWorldMapFileSize();

		BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE worldMapFile;
		auto worldMapRes = worldMapFile.OpenWrite(mapFilepath, (uint32_t)mapFileSize);
		if (BlitzenPlatform::CHECK_BLIT_MMF_RES_FOR_ERROR(worldMapRes))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_OPENING_FILE;
		}

		WorldMapHeader worldMapHeader;
		size_t offset = GCMapFileHeaderElementCount * sizeof(size_t);

		// Valid data counters in resident arrays
		worldMapHeader[WorldMapHeaderResidentCount] = pWorldResidents->mResidentCount;
		worldMapHeader[WorldMapHeaderWorldVariableCount] = pWorldResidents->mWorldVariableCount;
		worldMapHeader[WorldMapHeaderTransparentRenderCount] = pWorldResidents->m_renders.m_transparentStaticCount;
		worldMapHeader[WorldMapHeaderStaticRenderCount] = pWorldResidents->m_renders.m_opaqueStaticCount;
		worldMapHeader[WorldMapHeaderWorldVariablesWithVelocityCount] = pWorldResidents->mWithVelocityCount;
		worldMapHeader[WorldMapHeaderWorldVariablesWithGravityCount] = pWorldResidents->mWithGravityCount;

		size_t renderObjectsWriteSize = sizeof(RenderObject) * (BLIT_MAX_WORLD_VARIABLE_COUNT + pWorldResidents->m_renders.m_opaqueStaticCount);
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, renderObjectsWriteSize, pWorldResidents->m_renders.m_renders))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_RENDER_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapHeaderRenderObjectsID] = offset;
		offset += renderObjectsWriteSize;

		size_t worldTransformsWriteSize = sizeof(MeshTransform) * (BLIT_MAX_WORLD_VARIABLE_COUNT + pWorldResidents->m_renders.m_opaqueStaticCount);
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, worldTransformsWriteSize, pWorldResidents->mTransforms.m_transforms))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_TRANSFORM_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapHeaderWorldTransformsID] = offset;
		offset += worldTransformsWriteSize;

		size_t worldVariableTransformsWriteSize = sizeof(WVTransform) * pWorldResidents->mWorldVariableCount;
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, worldVariableTransformsWriteSize, pWorldResidents->WVTransforms))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLE_TRANSFORM_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapHeaderWorldVariableTransformsID] = offset;
		offset += worldVariableTransformsWriteSize;

		size_t worldVariableDataWriteSize = sizeof(WORLD_VARIABLE) * pWorldResidents->mWorldVariableCount;
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, worldVariableDataWriteSize, pWorldResidents->MWorldVariables))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLE_DATA_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapHeaderWorldVariablesDataID] = offset;
		offset += worldVariableDataWriteSize;
		
		size_t worldVariableVelocitiesWriteSize = sizeof(WVVelocity) * pWorldResidents->mWithGravityCount;
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, worldVariableVelocitiesWriteSize, pWorldResidents->WVVelocityData))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLE_VELOCITIES_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapHeaderWorldVariableVelocitiesID] = offset;
		offset += worldVariableVelocitiesWriteSize;

		size_t visibilityBoundingSpheresWriteSize = sizeof(BoundingSphere) * (BLIT_MAX_WORLD_VARIABLE_COUNT + pWorldResidents->m_renders.m_opaqueStaticCount);
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, visibilityBoundingSpheresWriteSize, pWorldResidents->MColliders.m_boundingSpheres))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_VISIBILITY_BOUNDING_SPHERES_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapHeaderVisibilityBoundingSpheresID] = offset;
		offset += visibilityBoundingSpheresWriteSize;

		size_t collidersAMaxRadWriteSize = sizeof(ColliderAMaxRad) * (BLIT_MAX_WORLD_VARIABLE_COUNT + pWorldResidents->mTransforms.m_staticTransformCount);
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, collidersAMaxRadWriteSize, pWorldResidents->MColliders.MColliderAMaxRad))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_COLLIDER_AMAXRAD_DATA_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapHeaderCollidersAMaxRadID] = offset;
		offset += collidersAMaxRadWriteSize;

		size_t collidersBMinTypeWriteSize = sizeof(ColliderBMinType) * (BLIT_MAX_WORLD_VARIABLE_COUNT + pWorldResidents->mTransforms.m_staticTransformCount);
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, collidersBMinTypeWriteSize, pWorldResidents->MColliders.MColliderBMinType))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_COLLIDER_BMINTYPE_DATA_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapHeaderCollidersBMinTypeID] = offset;
		offset += collidersBMinTypeWriteSize;

		// This could be skipped as this array array is just a result of a per transform on the AMaxRad data
		size_t transformedCollidersAMaxRadWriteSize = sizeof(ColliderAMaxRad) * pWorldResidents->mWorldVariableCount;
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, transformedCollidersAMaxRadWriteSize, pWorldResidents->MColliders.MTransformedColliderAMaxRad))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_TRANSFORMED_COLLIDER_AMAXRAD_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapHeaderCollidersTransformedAMaxRadID] = offset;
		offset += transformedCollidersAMaxRadWriteSize;

		// This could be skipped as this array array is just a result of a per transform on the AMaxRad data
		size_t transformedCollidersBMinTypeWriteSize = sizeof(ColliderBMinType) * pWorldResidents->mWorldVariableCount;
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, transformedCollidersBMinTypeWriteSize, pWorldResidents->MColliders.MTransformedColliderBMinType))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_TRANSFORMED_COLLIDER_BMINTYPE_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapHeaderCollidersTransformedBMinTypeID] = offset;
		offset += transformedCollidersBMinTypeWriteSize;

		size_t colliderWorldEffectsWriteSize = sizeof(ColliderWorldEffects) * pWorldResidents->mWorldVariableCount;
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, colliderWorldEffectsWriteSize, pWorldResidents->MColliders.WVColliderHitData))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_COLLIDER_WORLD_EFFECTS_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapHeaderCollidersWorldEffectsID] = offset;
		offset += colliderWorldEffectsWriteSize;

		size_t colliderTemporalCountersWriteSize = sizeof(uint32_t) * pWorldResidents->mWorldVariableCount;
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, colliderTemporalCountersWriteSize, pWorldResidents->MColliders.wvAllowedTemporalCount))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_COLLIDER_TEMPORAL_COUNTERS_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapheaderCollidersTemporalDataCountersID] = offset;
		offset += colliderTemporalCountersWriteSize;

		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, GCBlitStartOfFileOffset, sizeof(size_t) * GCMapFileHeaderElementCount, worldMapHeader))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_HEADER;
		}

		return UPLOAD_WORLD_MAP_RES::SUCCESS;
	}

	bool LoadWORLDMapResourceNamesFromDisk(const char* mapName, BlitCL::String* names, size_t* nameLengths)
	{
		BlitCL::String stringContainer;
		const char* filepath = BuildWorldMapResourceNamesFilepath(stringContainer, mapName);

		BlitzenPlatform::C_FILE_SCOPE scopedFile;
		if (!BlitzenPlatform::FilepathExists(filepath))
		{
			BLIT_ERROR("%s: Filepath for map resource names was never created", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}

		constexpr bool LCBinaryFileFlag = false;
		if (!scopedFile.Open(filepath, BlitzenPlatform::FileModes::Read, LCBinaryFileFlag))
		{
			BLIT_ERROR("%s: Failed to load file for map resource names read", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}

		size_t tempLength = 0;
		uint32_t index = 0;
		while (BlitzenPlatform::FilesystemReadLine(scopedFile, GCResourceNameMaxSize, names[index].GetDataPointer(), &tempLength))
		{
			nameLengths[index++] = tempLength;
		}

		return true;

	}

	bool UploadWORLDMapResourceNamesToDisk(const char* mapName, const BlitCL::String* names, uint32_t nameCount)
	{
		BlitCL::String stringContainer;
		const char* filepath = BuildWorldMapResourceNamesFilepath(stringContainer, mapName);

		BlitzenPlatform::C_FILE_SCOPE scopedFile;
		constexpr bool LCBinaryFileFlag = false;
		if (!scopedFile.Open(filepath, BlitzenPlatform::FileModes::Write, LCBinaryFileFlag))
		{
			return false;
		}

		for (uint32_t f = 0; f < nameCount; ++f)
		{
			if (!BlitzenPlatform::FilesystemWriteLine(scopedFile, names[f].GetClassic()))
			{
				return false;
			}
		}

		return true;
	}
}