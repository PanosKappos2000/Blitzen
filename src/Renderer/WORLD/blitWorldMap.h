#pragma once
#include "Core/blitzenEngine.h"
#include "Renderer/Entities/Residents/blitResidentManager.h"
#include "BlitCL/blitString.h"
#include "Platform/Filesystem/blitCFILE.h"
#include "Platform/Common/blitMappedFile.h"
#include "Renderer/Resources/Textures/blitTextures.h"

namespace BlitzenEngine
{
	constexpr const char* GCDefaultWorldMapName = "BlitzenDemoMap";
	constexpr const char* GCWorldMapFileExtension = ".blitMap";
	constexpr const char* GCWorldMapResourceNamesFileExtension = ".bmstr";
	constexpr const char* GCClientWorldMapDirectory = BLITZEN_CLIENT_WORLDMAPS_DIRECTORY;
	constexpr uint32_t GCResourceNameMaxSize = 100;
	constexpr uint32_t GCResourceNameMaxCount = 1000;
	constexpr uint32_t GCSceneNameMaxCount = 100;
	constexpr uint32_t GCSceneNameMaxSize = 100;
	constexpr uint32_t GCMapFileHeaderElementCount = 20;
	constexpr uint32_t GCWorldMapResourceOffsetsHeaderElementCount = 4;
	constexpr const char* GCNameOfWorldMapTextureNamesBINSTRFile = "textureNames.bbinstr";
	constexpr const char* GCWorldMapMaterialBatchBMSTRFileName = "matBatch.bmstr";
	constexpr const char* GCNameOfWorldResourcesOffsetsBinaryFile = "resourcesOffsets.bin";

	// One of the functions needed during project initialization
	// Create the world map directory for the first time
	BLIT_OFFLINE_FUNC bool CreateWorldMapDirectory(const char* mapName);

	enum class BMSTRFileReadRes
	{
		Read,
		End,
		Error,
	};

	enum WorldMapHeaderIndices : uint32_t
	{
		WorldMapHeaderResidentCount = 0,
		WorldMapHeaderWorldVariableCount = 1,
		WorldMapHeaderTransparentRenderCount = 2,
		WorldMapHeaderStaticRenderCount = 3,
		WorldMapHeaderWorldVariablesWithVelocityCount = 4,
		WorldMapHeaderWorldVariablesWithGravityCount = 5,
		WorldMapHeaderRenderObjectsID = 6,
		WorldMapHeaderWorldTransformsID = 7,
		WorldMapHeaderWorldVariableTransformsID = 8,
		WorldMapHeaderWorldVariablesDataID = 9,
		WorldMapHeaderWorldVariableVelocitiesID = 10,
		WorldMapHeaderWorldVariableGravityDataID = 11,
		WorldMapHeaderWorldVariablesWithGravityIndicesID = 12,
		WorldMapHeaderVisibilityBoundingSpheresID = 13,
		WorldMapHeaderCollidersAMaxRadID = 14,
		WorldMapHeaderCollidersBMinTypeID = 15,
		WorldMapHeaderCollidersTransformedAMaxRadID = 16,
		WorldMapHeaderCollidersTransformedBMinTypeID = 17,
		WorldMapHeaderCollidersWorldEffectsID = 18,
		WorldMapheaderCollidersTemporalDataCountersID = 19,

		WorldMapHeaderMax = 20,
	};
	static_assert(WorldMapHeaderMax == GCMapFileHeaderElementCount);
	using WorldMapHeader = size_t[GCMapFileHeaderElementCount];

	enum class LOAD_WORLD_MAP_RES : int64_t
	{
		SUCCESS = BlitzenCore::CE_BLITZEN_SUCCESS,
		FATAL = BlitzenCore::CE_BLITZEN_FATAL,

		WORLD_MAP_FILE_NEVER_CREATED = -1,
		ERROR_OPENING_FILE = -2,
		FAILED_TO_LOAD_HEADER = -3,
		ERROR_READING_RENDER_OBJECT_ARRAY = -4,
		ERROR_READING_WORLD_TRANSFORM_ARRAY = -5,
		ERROR_READING_WORLD_VARIABLE_TRANSFORM_ARRAY = -6,
		ERROR_READING_WORLD_VARIABLE_TYPEID_ARRAY = -7,
		ERROR_READING_WORLD_VARIABLE_VELOCITIES_ARRAY = -8,
		ERROR_READING_VISIBILITY_BOUNDING_SPHERE_ARRAY = -9,
		ERROR_READING_COLLIDER_AMAXRAD_DATA_ARRAY = -10,
		ERROR_READING_COLLIDER_BMINTYPE_DATA_ARRAY = -11,
		ERROR_READING_TRANSFORMED_COLLIDER_AMAXRAD_DATA_ARRAY = -12,
		ERROR_READING_TRANSFORMED_COLLIDER_BMINTYPE_DATA_ARRAY = -13,
		ERROR_READING_COLLIDER_WORLD_EFFECTS_ARRAY = -14,
		ERROR_READING_COLLIDER_TEMPORAL_COUNTERS_ARRAY = -15,
		ERROR_READING_WORLD_VARIABLE_GRAVITY_DATA_ARRAY = -16,
		ERROR_READING_WORLD_VARIABLES_WITH_GRAVITY_INDICES_ARRAY = -17,
	};
	// Opens the file found at the filepath and maps its memory
	// Each array inside is ready and place in the residents
	LOAD_WORLD_MAP_RES LoadWORLDMapFromDisk(const char* mapName, WORLD_RESIDENTS* pWorldResidents);

	constexpr uint32_t GCLoadWORLDMapResourceNamesFromDiskErrorCode = GCResourceNameMaxCount;
	// Reads resource names for the given map. Returns the error code above is something goes wrong.
	uint32_t LoadWORLDMapResourceNamesFromDisk(const char* mapName, BlitCL::String* names, size_t* nameLengths);

	// Loads the resource name found at the current index of the bmstrFile parameter
	// Places it into the char** buffer. If something goes wrong it returns error.
	// Read and End are both valid, but End means that there are no more resources.
	BMSTRFileReadRes ReadBmstrFileNextLine(BlitzenPlatform::C_FILE_SCOPE& bmstrFile, char** buffer);
	
	// Finds all the resources inside a scene file and writes their names to the Bmstr file
	// This lets a map load the scene's resources by just looking at the resources Bmstr file
	bool AddSceneResourcesToWorldMapResourceBmstrFile(BlitzenPlatform::C_FILE_SCOPE& bmstrFile, const char* sceneName, uint32_t resourceCount);

	constexpr uint32_t GCGetResourceIDFromWORLDMapResourceFileErrorCode = GCResourceNameMaxCount;
	// Seeks resource name in map file. Returns index when it is found. Error code if it is never found.
	uint32_t GetResourceIDFromWORLDMapResourceFile(const char* resourceName, const char* mapName);

	constexpr uint32_t GCLoadWORLDMapSceneNamesFromDiskErrorCode = GCSceneNameMaxCount;
	// Loads all scene names for a given map
	uint32_t LoadWORLDMapSceneNamesFromDisk(const char* mapName, BlitCL::String* names, size_t* nameLengths);

	//---------------------------------------------------------------------------------------------------------------------------------
	// WORLD map resources offsets. Allows for all world maps to use the same resources, 
	// by keeping offsets for the indices used by the structs
	//---------------------------------------------------------------------------------------------------------------------------------
	enum WorldMapResourcesOffsetsHeaderIndices
	{
		WorldMapResourcesOffsetsHeaderMaterialCountID = 0,
		WorldMapResourcesOffsetsHeaderGeometryCountID = 1,
		WorldMapResourcesOffsetsHeaderGeometryOffsetsID = 2,
		WorldMapResourcesOffsetsHeaderMaterialOffsetsID = 3,

		WorldMapResourcesOffsetsHeaderMax = 4
	};
	static_assert(WorldMapResourcesOffsetsHeaderMax == GCWorldMapResourceOffsetsHeaderElementCount);
	using WorldMapResourcesOffsetsHeader = size_t[GCWorldMapResourceOffsetsHeaderElementCount];

	BLIT_OFFLINE_FUNC bool OpenWorldMapResourcesContextFileForWriting(BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE& file, const char* mapName);

	BLIT_OFFLINE_FUNC bool UploadWorldMapResourceContextToDisk(BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE& file, uint32_t* materialTextureOffsets, uint32_t materialTextureOffsetCount, 
		uint32_t* geometryMaterialOffsets, uint32_t geometryMaterialOffsetCount);

	bool LoadWorldMapResourcesContextFromDisk(const char* mapName, BlitzenCore::BLIT_PTR& materialTextureOffsets, BlitzenCore::BLIT_PTR& geometryMaterialOffsets);

	enum class UPLOAD_WORLD_MAP_RES : int64_t
	{
		SUCCESS = BlitzenCore::CE_BLITZEN_SUCCESS,
		FATAL = BlitzenCore::CE_BLITZEN_FATAL,

		ERROR_OPENING_FILE = -1,
		ERROR_WRITING_RENDER_STRUCTURE_OF_ARRAYS = -2,
		ERROR_WRITING_WORLD_TRANSFORM_STRUCTURE_OF_ARRAYS = -3,
		ERROR_WRITING_WORLD_VARIABLE_TRANSFORM_STRUCTURE_OF_ARRAYS = -4,
		ERROR_WRITING_WORLD_VARIABLE_DATA_STRUCTURE_OF_ARRAYS = -5,
		ERROR_WRITING_WORLD_VARIABLE_VELOCITIES_STRUCTURE_OF_ARRAYS = -6,
		ERROR_WRITING_VISIBILITY_BOUNDING_SPHERES_STRUCTURE_OF_ARRAYS = -7,
		ERROR_WRITING_COLLIDER_AMAXRAD_DATA_STRUCTURE_OF_ARRAYS = -8,
		ERROR_WRITING_COLLIDER_BMINTYPE_DATA_STRUCTURE_OF_ARRAYS = -9,
		ERROR_WRITING_TRANSFORMED_COLLIDER_AMAXRAD_STRUCTURE_OF_ARRAYS = -10,
		ERROR_WRITING_TRANSFORMED_COLLIDER_BMINTYPE_STRUCTURE_OF_ARRAYS = -11,
		ERROR_WRITING_COLLIDER_WORLD_EFFECTS_STRUCTURE_OF_ARRAYS = -12,
		ERROR_WRITING_COLLIDER_TEMPORAL_COUNTERS_STRUCTURE_OF_ARRAYS = -13,
		ERROR_WRITING_HEADER = -14,
		ERROR_WRITING_WORLD_VARIABLE_GRAVITY_DATA_ARRAY = -15,
		ERROR_WRITING_WORLD_VARIABLES_WITH_GRAVITY_INDICES_ARRAY = -16,
	};
	UPLOAD_WORLD_MAP_RES UploadWORLDMapToDisk(const char* mapName, WORLD_RESIDENTS* pWorldResidents);

	BLIT_OFFLINE_FUNC bool UpdateWorldMapResources(const char* filepath);

	bool UpdateWorldMapResidents(const char* filepath);

	inline size_t GetWorldMapFileSize()
	{
		return sizeof(WORLD_RESIDENTS) + sizeof(GCResourceNameMaxSize) * GCResourceNameMaxCount + sizeof(size_t) * GCMapFileHeaderElementCount;
	}

	// Opens the file that holds the names of all resources
	// This is to be done when the map if first loaded to keep track of additional names
	bool OpenResourceNamesBMSTRFile(const char* mapName, BlitzenPlatform::C_FILE_SCOPE& file);

	// Opens the File that holds resource names in read mode
	// The names will be read one by one to load the actual resources
	bool OpenWorldMapBmstrFileForResourceNameReadback(BlitzenPlatform::C_FILE_SCOPE& bmstFile, const char* mapName);

	// Uploads single resource names to the bmst file that will be responsible for finding resources during load
	BLIT_OFFLINE_FUNC bool UploadWORLDMapResourceNamesToDisk(const char* mapName, const BlitCL::String* names, uint32_t nameCount);

	BLIT_OFFLINE_FUNC bool UploadWORLDMapResourceNameToDisk(const char* resourceName, BlitzenPlatform::C_FILE_SCOPE& file);

	// Uploads scene names to the bmstr file that will be responsible for finding scene resources during load
	BLIT_OFFLINE_FUNC bool UploadWORLDMapSceneNamesToDisk(const char* mapName, const BlitCL::String* names, uint32_t nameCount);

	bool OpenMaterialBatchBMSTRFile(const char* mapName, BlitzenPlatform::C_FILE_SCOPE& file);

	BLIT_OFFLINE_FUNC bool UploadMaterialBatchNameToDisk(BlitzenPlatform::C_FILE_SCOPE& file, const char* sceneName);

	bool OpenMaterialBatchNamesBmstrFileForRead(BlitzenPlatform::C_FILE_SCOPE& file, const char* mapName);

	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	// Binary String file, part of Blitzen's custom files (BINSTR).
	// Uses memory mapped files to read and write a pool of strings.
	// Accesses it by using the full pool size and the char count of each string.
	//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	constexpr uint32_t GCBinaryStringFileHeaderElementCount = 4;
	constexpr const char* GCBinaryStringFileExtension = ".bbinstr";
	enum BinaryStringFileHeaderIndices
	{
		BlitBINSTRFileHeaderStringCountID = 0,
		BlitBINSTRFileHeaderStringSizesOffsetID = 1,
		BlitBINSTRFileHeaderStringDataOffsetID = 2,
		BlitBINSTRFileHeaderStringBufferSizeID = 3,

		BlitBINSTRFileHeaderMax = 4
	};
	static_assert(GCBinaryStringFileHeaderElementCount == BlitBINSTRFileHeaderMax);
	using BinaryStringFileHeader = size_t[GCBinaryStringFileHeaderElementCount];

	BLIT_OFFLINE_FUNC bool OpenBINSTRFileForTextureNameWriting(BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE& file, const char* mapName);

	BLIT_OFFLINE_FUNC bool AddStringDataToBINSTRFile(BlitzenPlatform::MEMORY_MAPPED_FILE_SCOPE& file, char* stringData, size_t* stringSizes, uint32_t stringCount, uint32_t stringBufferSize);

	bool ReadStringDataFromBINSTRFile(const char* mapName, BlitzenCore::BLIT_PTR& outStringData, BlitzenCore::BLIT_PTR& outStringSize, uint32_t& outStringCount);

	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------
	// RESULT LOGGING
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------
	inline const char* GET_LOAD_WRLD_MAP_RES_ENUM_STRING(LOAD_WORLD_MAP_RES res)
	{
		switch (res)
		{
		case LOAD_WORLD_MAP_RES::SUCCESS: 															return "LOAD_WORLD_MAP_RES::SUCCESS";
		case LOAD_WORLD_MAP_RES::FATAL: 															return "LOAD_WORLD_MAP_RES::FATAL";
		case LOAD_WORLD_MAP_RES::WORLD_MAP_FILE_NEVER_CREATED:										return "LOAD_WORLD_MAP_RES::WORLD_MAP_FILE_NEVER_CREATED";
		case LOAD_WORLD_MAP_RES::ERROR_OPENING_FILE:												return "LOAD_WORLD_MAP_RES::ERROR_OPENING_FILE";
		case LOAD_WORLD_MAP_RES::FAILED_TO_LOAD_HEADER:												return "LOAD_WORLD_MAP_RES::FAILED_TO_LOAD_HEADER";
		case LOAD_WORLD_MAP_RES::ERROR_READING_RENDER_OBJECT_ARRAY:									return "LOAD_WORLD_MAP_RES::ERROR_READING_RENDER_OBJECT_ARRAY";
		case LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_TRANSFORM_ARRAY:								return "LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_TRANSFORM_ARRAY";
		case LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLE_TRANSFORM_ARRAY:						return "LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLE_TRANSFORM_ARRAY";
		case LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLE_TYPEID_ARRAY:							return "LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLE_TYPEID_ARRAY";
		case LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLE_VELOCITIES_ARRAY:						return "LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLE_VELOCITIES_ARRAY";
		case LOAD_WORLD_MAP_RES::ERROR_READING_VISIBILITY_BOUNDING_SPHERE_ARRAY:					return "LOAD_WORLD_MAP_RES::ERROR_READING_VISIBILITY_BOUNDING_SPHERE_ARRAY";
		case LOAD_WORLD_MAP_RES::ERROR_READING_COLLIDER_AMAXRAD_DATA_ARRAY:							return "LOAD_WORLD_MAP_RES::ERROR_READING_COLLIDER_AMAXRAD_DATA_ARRAY";
		case LOAD_WORLD_MAP_RES::ERROR_READING_TRANSFORMED_COLLIDER_AMAXRAD_DATA_ARRAY:				return "LOAD_WORLD_MAP_RES::ERROR_READING_TRANSFORMED_COLLIDER_AMAXRAD_DATA_ARRAY";
		case LOAD_WORLD_MAP_RES::ERROR_READING_TRANSFORMED_COLLIDER_BMINTYPE_DATA_ARRAY:			return "LOAD_WORLD_MAP_RES::ERROR_READING_TRANSFORMED_COLLIDER_BMINTYPE_DATA_ARRAY";
		case LOAD_WORLD_MAP_RES::ERROR_READING_COLLIDER_WORLD_EFFECTS_ARRAY:						return "LOAD_WORLD_MAP_RES::ERROR_READING_COLLIDER_WORLD_EFFECTS_ARRAY";
		case LOAD_WORLD_MAP_RES::ERROR_READING_COLLIDER_TEMPORAL_COUNTERS_ARRAY:					return "LOAD_WORLD_MAP_RES::ERROR_READING_COLLIDER_TEMPORAL_COUNTERS_ARRAY";
		case LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLE_GRAVITY_DATA_ARRAY:					return "LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLE_GRAVITY_DATA_ARRAY";
		case LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLES_WITH_GRAVITY_INDICES_ARRAY:			return "LOAD_WORLD_MAP_RES::ERROR_READING_WORLD_VARIABLES_WITH_GRAVITY_INDICES_ARRAY";
		default: 																					return "LOAD_WORLD_MAP_RES::UNKNOWN";
		}
	}

	inline const char* GET_UPLOAD_WRLD_MAP_RES_ENUM_STRING(UPLOAD_WORLD_MAP_RES res)
	{
		switch (res)
		{
		case UPLOAD_WORLD_MAP_RES::SUCCESS: 															return "UPLOAD_WORLD_MAP_RES::SUCCESS";
		case UPLOAD_WORLD_MAP_RES::FATAL: 																return "UPLOAD_WORLD_MAP_RES::FATAL";
		case UPLOAD_WORLD_MAP_RES::ERROR_OPENING_FILE: 													return "UPLOAD_WORLD_MAP_RES::ERROR_OPENING_FILE";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_RENDER_STRUCTURE_OF_ARRAYS:							return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_RENDER_STRUCTURE_OF_ARRAYS";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_TRANSFORM_STRUCTURE_OF_ARRAYS:					return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_TRANSFORM_STRUCTURE_OF_ARRAYS";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLE_TRANSFORM_STRUCTURE_OF_ARRAYS:			return "UPLOAD_WORLD_MAP_RES::ERROR_WRITE_WORLD_VARIABLE_TRANSFORM_STRUCTURE_OF_ARRAYS";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLE_DATA_STRUCTURE_OF_ARRAYS:				return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLE_DATA_STRUCTURE_OF_ARRAYS";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLE_VELOCITIES_STRUCTURE_OF_ARRAYS:			return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLE_VELOCITIES_STRUCTURE_OF_ARRAYS";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_VISIBILITY_BOUNDING_SPHERES_STRUCTURE_OF_ARRAYS:		return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_VISIBILITY_BOUNDING_SPHERES_STRUCTURE_OF_ARRAYS";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_COLLIDER_AMAXRAD_DATA_STRUCTURE_OF_ARRAYS:				return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_COLLIDER_AMAXRAD_DATA_STRUCTURE_OF_ARRAYS";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_COLLIDER_BMINTYPE_DATA_STRUCTURE_OF_ARRAYS:			return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_COLLIDER_BMINTYPE_DATA_STRUCTURE_OF_ARRAYS";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_TRANSFORMED_COLLIDER_AMAXRAD_STRUCTURE_OF_ARRAYS:		return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_TRANSFORMED_COLLIDER_AMAXRAD_STRUCTURE_OF_ARRAYS";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_TRANSFORMED_COLLIDER_BMINTYPE_STRUCTURE_OF_ARRAYS:		return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_TRANSFORMED_COLLIDER_BMINTYPE_STRUCTURE_OF_ARRAYS";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_COLLIDER_WORLD_EFFECTS_STRUCTURE_OF_ARRAYS:			return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_COLLIDER_WORLD_EFFECTS_STRUCTURE_OF_ARRAYS";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_COLLIDER_TEMPORAL_COUNTERS_STRUCTURE_OF_ARRAYS:		return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_COLLIDER_TEMPORAL_COUNTERS_STRUCTURE_OF_ARRAYS";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_HEADER:												return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_HEADER";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLE_GRAVITY_DATA_ARRAY:						return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLE_GRAVITY_DATA_ARRAY";
		case UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLES_WITH_GRAVITY_INDICES_ARRAY:			return "UPLOAD_WORLD_MAP_RES::ERROR_WRITING_WORLD_VARIABLES_WITH_GRAVITY_INDICES_ARRAY";
		default: 																						return "UPLOAD_WORLD_MAP_RES::UNKNOWN";
		}
	}

	enum class BlitTextureFileHeaderIndices : uint32_t
	{
		BlitTextureCountID = 0,
		BlitTextureNameSizesOffsetID = 1,
		BlitTextureNamesOffsetID = 2,

		BlitTextureFileHeaderMax = 3
	};
	using BlitTextureFileHeader = size_t[GCMaxLoadedTextureCount];

	bool AddTextureToWorldMapTextureFile(const char* mapName, const char* textureName);
}