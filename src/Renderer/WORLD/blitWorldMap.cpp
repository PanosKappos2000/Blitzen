#include "blitWorldMap.h"
#include "Platform/Common/blitMappedFile.h"
#include "Core/DbLog/blitAssert.h"
#include "Core/DbLog/blitLogger.h"
#include "Renderer/Resources/RapidFile/blitResourceRPF.h"

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

	static const char* BuildWorldMapSceneNamesFilepath(BlitCL::String& container, const char* mapName)
	{
		if (container.GetSize() != 0)
		{
			BLIT_WARN("%s: The string container passed for bmstr filepath was not empty", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			container.Clear();
		}

		container.Append(const_cast<char*>(GCClientWorldMapDirectory));
		container.Append(const_cast<char*>(mapName));
		container.Append("Scenes");
		container.Append(const_cast<char*>(GCWorldMapResourceNamesFileExtension));

		return container.GetClassic();
	}

	bool OpenResourceNamesBMSTRFile(const char* mapName, BlitzenPlatform::C_FILE_SCOPE& file)
	{
		BlitCL::FatString nameContainer{ strlen(GCClientWorldMapDirectory) + strlen(mapName) + strlen(GCWorldMapResourceNamesFileExtension)};
		nameContainer.Format("%s%s%s", GCClientWorldMapDirectory, mapName, GCWorldMapResourceNamesFileExtension);

		if (!BlitzenPlatform::FilepathExists(nameContainer.Get()))
		{
			if (!file.Open(nameContainer.Get(), BlitzenPlatform::FileModes::Write, BlitzenPlatform::GCFileBinaryFlagFalse))
			{
				BLIT_ERROR("%s: Failed to open resource names file for map '%s'", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
				return false;
			}

			return true;
		}

		if (!file.Open(nameContainer.Get(), BlitzenPlatform::FileModes::Append, BlitzenPlatform::GCFileBinaryFlagFalse))
		{
			BLIT_ERROR("%s: Failed to open resource names file for map '%s'", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
			return false;
		}

		return true;
	}

	bool OpenWorldMapBmstrFileForResourceNameReadback(BlitzenPlatform::C_FILE_SCOPE& bmstrFile, const char* mapName)
	{
		BlitCL::FatString nameContainer{ strlen(GCClientWorldMapDirectory) + strlen(mapName) + strlen(GCWorldMapResourceNamesFileExtension) };
		nameContainer.Format("%s%s%s", GCClientWorldMapDirectory, mapName, GCWorldMapResourceNamesFileExtension);

		if (!bmstrFile.Open(nameContainer.Get(), BlitzenPlatform::FileModes::Read, BlitzenPlatform::GCFileBinaryFlagFalse))
		{
			BLIT_ERROR("%s: Failed to open resource names file for map %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
			return false;
		}

		return true;
	}

	bool OpenMaterialBatchBMSTRFile(const char* mapName, BlitzenPlatform::C_FILE_SCOPE& file)
	{
		BlitCL::FatString filepath{ strlen(GCClientWorldMapDirectory) + strlen(mapName) + strlen("/") + strlen(GCWorldMapMaterialBatchBMSTRFileName)};
		filepath.Format("%s%s/%s%s", GCClientWorldMapDirectory, mapName, GCWorldMapMaterialBatchBMSTRFileName);

		if (!BlitzenPlatform::FilepathExists(filepath.Get()))
		{
			if (!file.Open(filepath.Get(), BlitzenPlatform::FileModes::Write, BlitzenPlatform::GCFileBinaryFlagFalse))
			{
				BLIT_ERROR("%s: Failed to open material batch names files for map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
				return false;
			}

			return true;
		}

		if (!file.Open(filepath.Get(), BlitzenPlatform::FileModes::Append, BlitzenPlatform::GCFileBinaryFlagFalse))
		{
			BLIT_ERROR("%s: Failed to open material batch names files for map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
			return false;
		}

		return true;
	}

	bool UploadMaterialBatchNameToDisk(BlitzenPlatform::C_FILE_SCOPE& file, const char* sceneName)
	{
		if (!BlitzenPlatform::FilesystemWriteLine(file, sceneName))
		{
			BLIT_ERROR("%s: Failed to write material batch name to BMSTR file", BlitzenCore::CE_WORLD_SYSTEM_NAME)
			return false;
		}
		return true;
	}

	bool OpenMaterialBatchNamesBmstrFileForRead(BlitzenPlatform::C_FILE_SCOPE& file, const char* mapName)
	{
		BlitCL::FatString filepath{ strlen(GCClientWorldMapDirectory) + strlen(mapName) + strlen("/") + strlen(GCWorldMapMaterialBatchBMSTRFileName)};
		filepath.Format("%s%s/%s", GCClientWorldMapDirectory, mapName, GCWorldMapMaterialBatchBMSTRFileName);

		if (!file.Open(filepath.Get(), BlitzenPlatform::FileModes::Read, BlitzenPlatform::GCFileBinaryFlagFalse))
		{
			BLIT_ERROR("%s: Failed to open material batch file for map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
			return false;
		}

		return true;
	}

	BLIT_OFFLINE_FUNC bool OpenWorldMapResourcesContextFileForWriting(BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE& file, const char* mapName)
	{
		BlitCL::FatString filepath{ strlen(GCClientWorldMapDirectory) + strlen(mapName) + strlen("/") + strlen(GCNameOfWorldResourcesOffsetsBinaryFile) };
		filepath.Format("%s%s/%s", GCClientWorldMapDirectory, mapName, GCNameOfWorldResourcesOffsetsBinaryFile);

		bool firstLoadFlag = !BlitzenPlatform::FilepathExists(filepath.Get());

		uint32_t fileSize = GCMaxLoadedMaterialCount * sizeof(uint32_t) + BlitzenCore::Ce_MaxMeshPrimitivesCount * sizeof(uint32_t) + BlitzenCore::Ce_MaxMeshPrimitivesCount + 
			sizeof(size_t) * GCWorldMapResourceOffsetsHeaderElementCount;

		auto fileOpenRes = file.OpenGeneral(filepath.Get(), fileSize);
		if(BlitzenPlatform::CheckMmfResForError(fileOpenRes))
		{
			BLIT_ERROR("%s: Failed open resource context binary file for map \"%s\". Received Platform error: %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName, 
				BlitzenPlatform::GET_BLIT_MMF_RES_ERROR_STR(fileOpenRes));
			return false;
		}

		if (firstLoadFlag)
		{
			WorldMapResourcesOffsetsHeader header{};
			header[WorldMapResourcesOffsetsHeaderMaterialCountID] = 0;
			header[WorldMapResourcesOffsetsHeaderGeometryCountID] = 0;
			header[WorldMapResourcesOffsetsHeaderMaterialOffsetsID] = 0;
			header[WorldMapResourcesOffsetsHeaderGeometryOffsetsID] = 0;

			if (!BlitzenPlatform::WriteMemoryMappedFile(file, GCBlitStartOfFileOffset, sizeof(size_t) * GCWorldMapResourceOffsetsHeaderElementCount, header))
			{
				BLIT_ERROR("%s: Failed to write initial header to resource context binary file for map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
			}
		}

		return true;
	}

	bool UploadWorldMapResourceContextToDisk(BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE& file, uint32_t* materialTextureOffsets, uint32_t materialTextureOffsetCount,
		uint32_t* geometryMaterialOffsets, uint32_t geometryMaterialOffsetCount)
	{
		if (materialTextureOffsets == nullptr || materialTextureOffsetCount == 0)
		{
			BLIT_ERROR("%s: World Map Resource Context Binary File Write function received no texture offset data", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}

		if (geometryMaterialOffsets == nullptr || geometryMaterialOffsetCount == 0)
		{
			BLIT_ERROR("%s: World Map Resource Context Binary File Write function received no material offset data", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}

		WorldMapResourcesOffsetsHeader header{};
		if (!BlitzenPlatform::ReadMemoryMappedFile(file, GCBlitStartOfFileOffset, sizeof(size_t) * GCWorldMapResourceOffsetsHeaderElementCount, header))
		{
			BLIT_ERROR("%s: Failed to read World Map Resource Context Binary File Header for update write operations", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}

		header[WorldMapResourcesOffsetsHeaderGeometryCountID] += geometryMaterialOffsetCount;
		header[WorldMapResourcesOffsetsHeaderMaterialCountID] += materialTextureOffsetCount;

		uint32_t headerWriteSize = uint32_t(sizeof(size_t) * GCWorldMapResourceOffsetsHeaderElementCount);
		uint32_t materialTextureOffsetsWriteSize = uint32_t(sizeof(uint32_t) * materialTextureOffsetCount);
		uint32_t geometryMaterialOffsetsWriteSize = uint32_t(sizeof(uint32_t) * geometryMaterialOffsetCount);

		uint32_t offset = headerWriteSize;

		if (offset + header[WorldMapResourcesOffsetsHeaderMaterialOffsetsID] + materialTextureOffsetsWriteSize > offset + GCMaxLoadedMaterialCount * sizeof(uint32_t))
		{
			BLIT_ERROR("%s: Material texture data execeeded size limit in World Map Resource Context Binary File Write Function", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}
		uint32_t materialTextureOffsetsWriteOffset = offset + (uint32_t)header[WorldMapResourcesOffsetsHeaderMaterialOffsetsID];
		header[WorldMapResourcesOffsetsHeaderMaterialOffsetsID] = (size_t)materialTextureOffsetsWriteOffset + materialTextureOffsetsWriteSize;
		offset += GCMaxLoadedMaterialCount * sizeof(uint32_t);

		if (offset + header[WorldMapResourcesOffsetsHeaderGeometryOffsetsID] + geometryMaterialOffsetsWriteSize > offset + BlitzenCore::Ce_MaxMeshPrimitivesCount * sizeof(uint32_t))
		{
			BLIT_ERROR("%s: Geometry material data exceeded size limit in World Map Resource Context Binary File Write Function", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}
		uint32_t geometryMaterialOffsetsWriteOffset = offset + (uint32_t)header[WorldMapResourcesOffsetsHeaderGeometryOffsetsID];
		header[WorldMapResourcesOffsetsHeaderGeometryOffsetsID] = (size_t)geometryMaterialOffsetsWriteOffset + geometryMaterialOffsetsWriteSize;
		offset += BlitzenCore::Ce_MaxMeshPrimitivesCount * sizeof(uint32_t);

		if (!BlitzenPlatform::WriteMemoryMappedFile(file, GCBlitStartOfFileOffset, headerWriteSize, header))
		{
			BLIT_ERROR("%s: Failed to write header data to World Map Resource Context Binary File", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}

		if (!BlitzenPlatform::WriteMemoryMappedFile(file, materialTextureOffsetsWriteOffset, materialTextureOffsetsWriteSize, materialTextureOffsets))
		{
			BLIT_ERROR("%s: Failed to write material texture offsets data to World Map Resource Context Binary File", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}

		if (!BlitzenPlatform::WriteMemoryMappedFile(file, geometryMaterialOffsetsWriteOffset, geometryMaterialOffsetsWriteSize, geometryMaterialOffsets))
		{
			BLIT_ERROR("%s: Failed to write geometry material offsets data to World Map Resource Context Binary File", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}

		return true;
	}

	bool LoadWorldMapResourcesContextFromDisk(const char* mapName, BlitzenCore::BLIT_PTR& materialTextureOffsets, BlitzenCore::BLIT_PTR& geometryMaterialOffsets)
	{
		BlitCL::FatString filepath{ strlen(GCClientWorldMapDirectory) + strlen(mapName) + strlen("/") + strlen(GCNameOfWorldResourcesOffsetsBinaryFile) };
		filepath.Format("%s%s/%s", GCClientWorldMapDirectory, mapName, GCNameOfWorldResourcesOffsetsBinaryFile);

		BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE file;
		auto openFileRes = file.OpenRead(filepath.Get());
		if (BlitzenPlatform::CheckMmfResForError(openFileRes))
		{
			BLIT_FATAL("%s: Failed to open World Map Resource Context Binary File for world map \"%s\". Got platform error: %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName, BlitzenPlatform::GET_BLIT_MMF_RES_ERROR_STR(openFileRes));
			return false;
		}

		WorldMapResourcesOffsetsHeader header{};
		uint32_t headerWriteSize = sizeof(size_t) * GCWorldMapResourceOffsetsHeaderElementCount;
		uint32_t offset = GCBlitStartOfFileOffset;
		if (!BlitzenPlatform::ReadMemoryMappedFile(file, offset, headerWriteSize, header))
		{
			BLIT_FATAL("%s: Failed to read header from World Map Resource Context Binary File for world map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
			return false;
		}
		offset += headerWriteSize;

		uint32_t materialTextureOffsetsReadSize = (uint32_t)header[WorldMapResourcesOffsetsHeaderMaterialCountID] * sizeof(uint32_t);
		materialTextureOffsets.Init(materialTextureOffsetsReadSize);
		if (!BlitzenPlatform::ReadMemoryMappedFile(file, offset, materialTextureOffsetsReadSize, materialTextureOffsets.mPtr))
		{
			BLIT_FATAL("%s: Failed to read material texture offsets data from World Map Resource Context Binary File for world map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
			return false;
		}
		offset += GCMaxLoadedMaterialCount * sizeof(uint32_t);

		uint32_t geometryMaterialOffsetsReadSize = (uint32_t)header[WorldMapResourcesOffsetsHeaderGeometryCountID] * sizeof(uint32_t);
		geometryMaterialOffsets.Init(geometryMaterialOffsetsReadSize);
		if (!BlitzenPlatform::ReadMemoryMappedFile(file, offset, geometryMaterialOffsetsReadSize, geometryMaterialOffsets.mPtr))
		{
			BLIT_FATAL("%s: Failed to read geometry material offsets data from World Map Resource Context Binary File for world map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
			return false;
		}
		offset += BlitzenCore::Ce_MaxMeshPrimitivesCount * sizeof(uint32_t);

		return true;
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
		if (BlitzenPlatform::CheckMmfResForError(worldMapRes))
		{
			return LOAD_WORLD_MAP_RES::ERROR_OPENING_FILE;
		}

		WorldMapHeader worldMapHeader;
		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, GCBlitStartOfFileOffset, sizeof(size_t) * GCMapFileHeaderElementCount, worldMapHeader))
		{
			return LOAD_WORLD_MAP_RES::FAILED_TO_LOAD_HEADER;
		}

		pWorldResidents->mResidentCount = (uint32_t)worldMapHeader[WorldMapHeaderResidentCount];
		pWorldResidents->mWorldVariableCount = (uint32_t)worldMapHeader[WorldMapHeaderWorldVariableCount];
		pWorldResidents->m_renders.m_transparentStaticCount = (uint32_t)worldMapHeader[WorldMapHeaderTransparentRenderCount];
		pWorldResidents->m_renders.m_opaqueStaticCount = (uint32_t)worldMapHeader[WorldMapHeaderStaticRenderCount];
		pWorldResidents->m_renders.m_opaqueDynamicCount = (uint32_t)worldMapHeader[WorldMapHeaderWorldVariableCount];
		pWorldResidents->MColliders.mStaticColliderCount = (uint32_t)worldMapHeader[WorldMapHeaderStaticRenderCount];
		pWorldResidents->MColliders.mWorldVariableColliderCount = (uint32_t)worldMapHeader[WorldMapHeaderWorldVariableCount];
		pWorldResidents->mWithVelocityCount = (uint32_t)worldMapHeader[WorldMapHeaderWorldVariablesWithVelocityCount];
		pWorldResidents->mWithGravityCount = (uint32_t)worldMapHeader[WorldMapHeaderWorldVariablesWithGravityCount];

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapHeaderRenderObjectsID], 
			sizeof(RenderObject) * (BLIT_MAX_WORLD_VARIABLE_COUNT + pWorldResidents->m_renders.m_opaqueStaticCount), pWorldResidents->m_renders.m_renders))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_RENDER_OBJECT_ARRAY;
		}

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapHeaderWorldTransformsID],
			sizeof(MeshTransform) * (BLIT_MAX_WORLD_VARIABLE_COUNT + pWorldResidents->m_renders.m_opaqueStaticCount), pWorldResidents->mTransforms.m_transforms))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_TRANSFORM_ARRAY;
		}

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapHeaderWorldVariableTransformsID],
			sizeof(WVTransform) * pWorldResidents->mWorldVariableCount, pWorldResidents->WVTransforms))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLE_TRANSFORM_ARRAY;
		}

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapHeaderWorldVariablesDataID],
			sizeof(WORLD_VARIABLE) * pWorldResidents->mWorldVariableCount, pWorldResidents->mWorldVariableTypes))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLE_TYPEID_ARRAY;
		}

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapHeaderWorldVariableVelocitiesID],
			sizeof(WVVelocity) * pWorldResidents->mWorldVariableCount, pWorldResidents->WVVelocityData))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLE_VELOCITIES_ARRAY;
		}

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapHeaderWorldVariableGravityDataID],
			sizeof(WVGravity) * pWorldResidents->mWithGravityCount, pWorldResidents->WVGravityData))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLE_GRAVITY_DATA_ARRAY;
		}

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapHeaderWorldVariablesWithGravityIndicesID],
			sizeof(Resident) * pWorldResidents->mWithGravityCount, pWorldResidents->WVWithGravityIDXs))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLES_WITH_GRAVITY_INDICES_ARRAY;
		}

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapHeaderVisibilityBoundingSpheresID],
			sizeof(BoundingSphere) * (BLIT_MAX_WORLD_VARIABLE_COUNT + pWorldResidents->m_renders.m_opaqueStaticCount), pWorldResidents->MColliders.m_boundingSpheres))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_VISIBILITY_BOUNDING_SPHERE_ARRAY;
		}

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapHeaderCollidersAMaxRadID],
			sizeof(ColliderAMaxRad) * (BLIT_MAX_WORLD_VARIABLE_COUNT + pWorldResidents->mTransforms.m_staticTransformCount), pWorldResidents->MColliders.MColliderAMaxRad))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_COLLIDER_AMAXRAD_DATA_ARRAY;
		}

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapHeaderCollidersBMinTypeID],
			sizeof(ColliderBMinType) * (BLIT_MAX_WORLD_VARIABLE_COUNT + pWorldResidents->mTransforms.m_staticTransformCount), pWorldResidents->MColliders.MColliderBMinType))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_COLLIDER_BMINTYPE_DATA_ARRAY;
		}

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapHeaderCollidersTransformedAMaxRadID],
			sizeof(ColliderAMaxRad) * pWorldResidents->mWorldVariableCount, pWorldResidents->MColliders.MTransformedColliderAMaxRad))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_TRANSFORMED_COLLIDER_AMAXRAD_DATA_ARRAY;
		}

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapHeaderCollidersTransformedBMinTypeID],
			sizeof(ColliderBMinType) * pWorldResidents->mWorldVariableCount, pWorldResidents->MColliders.MTransformedColliderBMinType))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_TRANSFORMED_COLLIDER_BMINTYPE_DATA_ARRAY;
		}

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapHeaderCollidersWorldEffectsID],
			sizeof(ColliderWorldEffects) * pWorldResidents->mWorldVariableCount, pWorldResidents->MColliders.WVColliderHitData))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_COLLIDER_WORLD_EFFECTS_ARRAY;
		}

		if (!BlitzenPlatform::ReadMemoryMappedFile(worldMapFile, worldMapHeader[WorldMapheaderCollidersTemporalDataCountersID],
			sizeof(uint32_t) * pWorldResidents->mWorldVariableCount, pWorldResidents->MColliders.wvAllowedTemporalCount))
		{
			return LOAD_WORLD_MAP_RES::ERROR_READING_COLLIDER_TEMPORAL_COUNTERS_ARRAY;
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
		if (BlitzenPlatform::CheckMmfResForError(worldMapRes))
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
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, worldVariableDataWriteSize, pWorldResidents->mWorldVariableTypes))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLE_DATA_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapHeaderWorldVariablesDataID] = offset;
		offset += worldVariableDataWriteSize;
		
		size_t worldVariableVelocitiesWriteSize = sizeof(WVVelocity) * pWorldResidents->mWorldVariableCount;
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, worldVariableVelocitiesWriteSize, pWorldResidents->WVVelocityData))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLE_VELOCITIES_STRUCTURE_OF_ARRAYS;
		}
		worldMapHeader[WorldMapHeaderWorldVariableVelocitiesID] = offset;
		offset += worldVariableVelocitiesWriteSize;

		size_t worldVariableGravityDataWriteSize = sizeof(WVGravity) * pWorldResidents->mWithGravityCount;
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, worldVariableGravityDataWriteSize, pWorldResidents->WVGravityData))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLE_GRAVITY_DATA_ARRAY;
		}
		worldMapHeader[WorldMapHeaderWorldVariableGravityDataID] = offset;
		offset += worldVariableGravityDataWriteSize;

		size_t worldVariablesWithGravityIndicesWriteSize = sizeof(Resident) * pWorldResidents->mWithGravityCount;
		if (!BlitzenPlatform::WriteMemoryMappedFile(worldMapFile, offset, worldVariablesWithGravityIndicesWriteSize, pWorldResidents->WVWithGravityIDXs))
		{
			return UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLES_WITH_GRAVITY_INDICES_ARRAY;
		}
		worldMapHeader[WorldMapHeaderWorldVariablesWithGravityIndicesID] = offset;
		offset += worldVariablesWithGravityIndicesWriteSize;

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

	uint32_t LoadWORLDMapResourceNamesFromDisk(const char* mapName, BlitCL::String* names, size_t* nameLengths)
	{
		BlitCL::String stringContainer;
		const char* filepath = BuildWorldMapResourceNamesFilepath(stringContainer, mapName);

		BlitzenPlatform::C_FILE_SCOPE scopedFile;
		if (!BlitzenPlatform::FilepathExists(filepath))
		{
			BLIT_ERROR("%s: Filepath for map resource names was never created", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return GCLoadWORLDMapResourceNamesFromDiskErrorCode;
		}

		constexpr bool LCBinaryFileFlag = false;
		if (!scopedFile.Open(filepath, BlitzenPlatform::FileModes::Read, LCBinaryFileFlag))
		{
			BLIT_ERROR("%s: Failed to load file for map resource names read", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return GCLoadWORLDMapResourceNamesFromDiskErrorCode;
		}

		size_t tempLength = 0;
		uint32_t index = 0;
		if (!scopedFile.m_pHandle || !names[index].GetDataPointer() || !*names[index].GetDataPointer()) return GCLoadWORLDMapResourceNamesFromDiskErrorCode;

		while (BlitzenPlatform::FilesystemReadLine(scopedFile, GCResourceNameMaxSize, names[index].GetDataPointer(), &tempLength))
		{
			nameLengths[index++] = tempLength;
			names[index].CopyString("");
		}

		return index;
	}

	BMSTRFileReadRes ReadBmstrFileNextLine(BlitzenPlatform::C_FILE_SCOPE& bmstrFile, char** buffer)
	{
		if (!bmstrFile.m_pHandle || !buffer || !*buffer) return BMSTRFileReadRes::Error;
		size_t size;
		// Since other possible errors have been check this can only return false if it is the end of the line
		if (!BlitzenPlatform::FilesystemReadLine(bmstrFile, GCResourceNameMaxSize, buffer, &size))
		{
			return BMSTRFileReadRes::End;
		}
		return BMSTRFileReadRes::Read;
	}

	bool AddSceneResourcesToWorldMapResourceBmstrFile(BlitzenPlatform::C_FILE_SCOPE& bmstrFile, const char* sceneName, uint32_t resourceCount)
	{
		BlitCL::FatString stringContainer{ strlen(sceneName) + strlen("/") + strlen("mesh") + 16 };
		for (uint32_t n = 0; n < resourceCount; n++)
		{
			stringContainer.Format("%s/mesh%u", sceneName, n);

			if (!BlitzenPlatform::FilesystemWriteLine(bmstrFile, stringContainer.Get()))
			{
				BLIT_ERROR("%s: Failed to write resource name %u from scene %s to world map resource .bmstr file", BlitzenCore::CE_WORLD_SYSTEM_NAME, n, sceneName);
				return false;
			}
		}

		return true;
	}

	uint32_t LoadWORLDMapSceneNamesFromDisk(const char* mapName, BlitCL::String* names, size_t* nameLengths)
	{
		BlitCL::String stringContainer;
		const char* filepath = BuildWorldMapSceneNamesFilepath(stringContainer, mapName);

		BlitzenPlatform::C_FILE_SCOPE scopedFile;
		if (!BlitzenPlatform::FilepathExists(filepath))
		{
			BLIT_ERROR("%s: Filepath for map scene names was never created", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return GCLoadWORLDMapSceneNamesFromDiskErrorCode;
		}

		if (!scopedFile.Open(filepath, BlitzenPlatform::FileModes::Read, BlitzenPlatform::GCFileBinaryFlagFalse))
		{
			BLIT_ERROR("%s: Failed to load file for map scene names read", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return GCLoadWORLDMapSceneNamesFromDiskErrorCode;
		}

		size_t tempLength = 0;
		uint32_t index = 0;
		if (!scopedFile.m_pHandle || !names[index].GetDataPointer() || !*names[index].GetDataPointer()) return GCLoadWORLDMapSceneNamesFromDiskErrorCode;

		while (BlitzenPlatform::FilesystemReadLine(scopedFile, GCSceneNameMaxSize, names[index].GetDataPointer(), &tempLength))
		{
			nameLengths[index++] = tempLength;
		}

		return index;
	}

	uint32_t GetResourceIDFromWORLDMapResourceFile(const char* resourceName, const char* mapName)
	{
		BlitCL::String nameBuffer{ GCResourceNameMaxSize };

		BlitCL::String stringContainer;
		const char* filepath = BuildWorldMapResourceNamesFilepath(stringContainer, mapName);

		BlitzenPlatform::C_FILE_SCOPE scopedFile;
		if (!BlitzenPlatform::FilepathExists(filepath))
		{
			return GCGetResourceIDFromWORLDMapResourceFileErrorCode;
		}

		constexpr bool LCBinaryFileFlag = false;
		if (!scopedFile.Open(filepath, BlitzenPlatform::FileModes::Read, BlitzenPlatform::GCFileBinaryFlagFalse))
		{
			return GCGetResourceIDFromWORLDMapResourceFileErrorCode;
		}

		size_t length = 0;
		uint32_t IDX = 0;
		// Goes through all lines 
		while (BlitzenPlatform::FilesystemReadLine(scopedFile, GCResourceNameMaxSize, nameBuffer.GetDataPointer(), &length))
		{
			// Returns line id if it finds the name
			if (strcmp(nameBuffer.GetClassic(), resourceName) == 0)
			{
				return IDX;
			}
			// Increments line id otherwise
			IDX++;
		}

		// Error if file ends
		return GCGetResourceIDFromWORLDMapResourceFileErrorCode;
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

	BLIT_OFFLINE_FUNC bool UploadWORLDMapResourceNameToDisk(const char* resourceName, BlitzenPlatform::C_FILE_SCOPE& file)
	{
		if(!BlitzenPlatform::FilesystemWriteLine(file, resourceName))
		{
			BLIT_ERROR("%s: Failed to write resource name '%s' to file", BlitzenCore::CE_WORLD_SYSTEM_NAME, resourceName);
			return false;
		}

		return true;
	}

	bool UploadWORLDMapSceneNamesToDisk(const char* mapName, const BlitCL::String* names, uint32_t nameCount)
	{
		BlitCL::String stringContainer;
		const char* filepath = BuildWorldMapSceneNamesFilepath(stringContainer, mapName);

		BlitzenPlatform::C_FILE_SCOPE scopedFile;
		if (!scopedFile.Open(filepath, BlitzenPlatform::FileModes::Write, BlitzenPlatform::GCFileBinaryFlagFalse))
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

	bool AddTextureToWorldMapTextureFile(const char* mapName, const char* textureName)
	{
		BlitCL::FatString filepath{ strlen(GCClientWorldMapDirectory) + strlen(mapName) + strlen("/") + strlen("textureNames.bin")};
		filepath.Format("%s%s/textureNames.bin", GCClientWorldMapDirectory, mapName);

		BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE textureFile;

		//auto texFileOpenRes = textureFile.OpenWrite(filepath.Get(), )
		
		return true;
	}

	bool OpenBINSTRFileForTextureNameWriting(BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE& file, const char* mapName)
	{
		BlitCL::FatString filepath{ strlen(GCClientWorldMapDirectory) + strlen(mapName) + strlen("/") + strlen(GCNameOfWorldMapTextureNamesBINSTRFile) };
		filepath.Format("%s%s/%s", GCClientWorldMapDirectory, mapName, GCNameOfWorldMapTextureNamesBINSTRFile);

		bool firstTimeLoadFlag = !BlitzenPlatform::FilepathExists(filepath.Get());

		uint32_t maxWriteSize = uint32_t(GCMaxLoadedTextureCount * GCWorldMapTextureNameMaxSize) + uint32_t(GCMaxLoadedTextureCount * sizeof(size_t)) +
			uint32_t(GCBinaryStringFileHeaderElementCount * sizeof(size_t));
		auto openFileRes = file.OpenGeneral(filepath.Get(), maxWriteSize);
		if (BlitzenPlatform::CheckMmfResForError(openFileRes))
		{
			BLIT_ERROR("%s: Failed to open World Map \"%s\" texture names binary string file", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
			return false;
		}

		if (firstTimeLoadFlag)
		{
			size_t blockOffset = 0;
			BinaryStringFileHeader header{};
			header[BlitBINSTRFileHeaderStringCountID] = 0;
			header[BlitBINSTRFileHeaderStringBufferSizeID] = 0;
			blockOffset += GCBinaryStringFileHeaderElementCount * sizeof(size_t);
			header[BlitBINSTRFileHeaderStringSizesOffsetID] = blockOffset;
			blockOffset += size_t(GCMaxLoadedTextureCount * sizeof(size_t));
			header[BlitBINSTRFileHeaderStringDataOffsetID] = blockOffset;
			if (!BlitzenPlatform::WriteMemoryMappedFile(file, 0, GCBinaryStringFileHeaderElementCount * sizeof(size_t), header))
			{
				BLIT_ERROR("%s: Failed to write header to texture names binary string file for World Map \"%s\"", BlitzenCore::CE_WORLD_SYSTEM_NAME, mapName);
				return false;
			}
		}

		return true;
	}

	bool UploadStringDataToBINSTRFile(BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE& file, char* stringData, size_t* stringSizes, uint32_t stringCount, uint32_t stringBufferSize,
		size_t stringSizesBlockSize, size_t stringDataBlockSize)
	{
		BLIT_ASSERT(stringData != nullptr && stringSizes != nullptr && stringCount != 0);

		BinaryStringFileHeader header{};
		size_t headerWriteSize = GCBinaryStringFileHeaderElementCount * sizeof(size_t);
		size_t headerWriteOffset = GCBlitStartOfFileOffset;
		if (!BlitzenPlatform::ReadMemoryMappedFile(file, headerWriteOffset, headerWriteSize, header))
		{
			BLIT_ERROR("%s: Failed to read header from Blitzen Binary String Format file for update writes", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}

		header[BlitBINSTRFileHeaderStringCountID] += stringCount;
		header[BlitBINSTRFileHeaderStringBufferSizeID] += stringBufferSize;

		size_t blockOffset = headerWriteSize;

		size_t stringSizesWriteSize = sizeof(size_t) * stringCount;
		size_t stringSizesWriteOffset = header[BlitBINSTRFileHeaderStringSizesOffsetID];
		if (stringSizesWriteOffset + stringSizesWriteSize > blockOffset + stringSizesBlockSize)
		{
			BLIT_ERROR("%s: String sizes array has overflown while writing to BINSTR file");
			return false;
		}
		header[BlitBINSTRFileHeaderStringSizesOffsetID] += stringSizesWriteSize;
		blockOffset += stringSizesBlockSize;
		
		size_t stringDataWriteOffset = header[BlitBINSTRFileHeaderStringDataOffsetID];
		if (stringDataWriteOffset + stringBufferSize > blockOffset + stringDataBlockSize)
		{
			BLIT_ERROR("%s: String data array has overflown while writing to BINSTR file");
			return false;
		}
		header[BlitBINSTRFileHeaderStringDataOffsetID] += stringBufferSize;
		blockOffset += stringDataBlockSize;

		if (!BlitzenPlatform::WriteMemoryMappedFile(file, GCBlitStartOfFileOffset, headerWriteSize, header))
		{
			BLIT_ERROR("%s: Failed to write to header of BINSTR file", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}

		if (!BlitzenPlatform::WriteMemoryMappedFile(file, stringSizesWriteOffset, stringSizesWriteSize, stringSizes))
		{
			BLIT_ERROR("%s: Failed to write string sizes to BINSTR file", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}

		if (!BlitzenPlatform::WriteMemoryMappedFile(file, stringDataWriteOffset, stringBufferSize, stringData))
		{
			BLIT_ERROR("%s: Failed to write string data to BINSTR file", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}

		return true;
	}

	bool LoadStringDataFromBINSTRFile(const char* filepath, BlitzenCore::BLIT_PTR& outStringData, BlitzenCore::BLIT_PTR& outStringSize, uint32_t& outStringCount, 
		uint32_t stringSizesBlockSize, uint32_t stringDataBlockSize)
	{
		BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE binstrFile;
		auto binstrFileOpenRes = binstrFile.OpenRead(filepath);
		if (BlitzenPlatform::CheckMmfResForError(binstrFileOpenRes))
		{
			BLIT_ERROR("%s: Failed to open binstr file. Received platform error: %s", BlitzenCore::CE_WORLD_SYSTEM_NAME, BlitzenPlatform::GET_BLIT_MMF_RES_ERROR_STR(binstrFileOpenRes));
			return false;
		}

		BinaryStringFileHeader header{};
		uint32_t offset = GCBlitStartOfFileOffset;
		if (!BlitzenPlatform::ReadMemoryMappedFile(binstrFile, offset, sizeof(size_t) * GCBinaryStringFileHeaderElementCount, header))
		{
			BLIT_ERROR("%s: Failed to read header of binstr file", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}
		offset += sizeof(size_t) * GCBinaryStringFileHeaderElementCount;

		outStringCount = (uint32_t)header[BlitBINSTRFileHeaderStringCountID];
		size_t stringBufferSize = header[BlitBINSTRFileHeaderStringBufferSizeID];
		outStringData.Init(stringBufferSize);
		outStringSize.Init(outStringCount * sizeof(size_t));

		if (!BlitzenPlatform::ReadMemoryMappedFile(binstrFile, offset, outStringCount * sizeof(size_t), outStringSize.mPtr))
		{
			BLIT_ERROR("%s: Failed to read string sizes array from binstr file", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}
		offset += stringSizesBlockSize;

		if (!BlitzenPlatform::ReadMemoryMappedFile(binstrFile, offset, stringBufferSize, outStringData.mPtr))
		{
			BLIT_ERROR("%s: Failed to read string buffer from binstr file", BlitzenCore::CE_WORLD_SYSTEM_NAME);
			return false;
		}

		char* test = reinterpret_cast<char*>(outStringData.mPtr);

		return true;
	}

	bool CreateWorldMapDirectory(const char* mapName)
	{
		BlitCL::FatString filepath{ strlen(GCClientWorldMapDirectory) + strlen(mapName) };
		filepath.Format("%s%s", GCClientWorldMapDirectory, mapName);
		return BlitzenPlatform::CreateDirectoryIfMissing(filepath.Get());
	}
}