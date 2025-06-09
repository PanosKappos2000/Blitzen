#pragma once

#include "blitEntityManager.h"
#include "Renderer/Interface/blitRenderer.h"

namespace BlitzenEngine
{
	struct BLIT_ENTITY_CREATE_CONTEXT
	{
		PFN_ENTITY_UPDATE m_updatePfn{};

		float m_rotationSpeed{ 0.f };

		MeshTransform initialTransform{};
	};

	enum class ENTITY_CREATE_RES : int8_t
	{
		SUCCESS = 0,

		ENTITIES_FULL = -1,
		DYNAMICALLY_UPDATED_ENTITIES_FULL = -2,
		DYNAMIC_ORIENTATION_FULL = -3,

		UNKNOWN = -126,
		MAX = -127
	};

	inline const char* GET_ENTITY_CREATE_RES_MSG(ENTITY_CREATE_RES res)
	{
		switch (res)
		{
		case ENTITY_CREATE_RES::SUCCESS: return "ENTITY_CREATE_RES_SUCCESS";
		case ENTITY_CREATE_RES::DYNAMICALLY_UPDATED_ENTITIES_FULL: return "ENTITY_CREATE_RES_DYNAMICALLY_UPDATED_ENTITIES_FULL";
		case ENTITY_CREATE_RES::ENTITIES_FULL: return "ENTITY_CREATE_RES_ENTITIES_FULL";
		case ENTITY_CREATE_RES::UNKNOWN: case ENTITY_CREATE_RES::MAX: default: return "ENTITY_CREATE_RES_UNKNOWN";
		}
	}

	inline bool LOG_ENTITY_CREATION_ERROR_MSG_AND_RETURN(ENTITY_CREATE_RES res)
	{
		if (BlitzenCore::BLIT_CHECK_FAIL(res))
		{
			BLIT_ERROR("Entity creation error result: %s", GET_ENTITY_CREATE_RES_MSG(res));
			return false;
		}

		BLIT_WARN("Entity creation, no error msg found. Error logging should not have been called");
		return false;
	}

	ENTITY_CREATE_RES AddEntityToWorld(EntityManager* pManager, MeshResources& meshes, const char* meshName, ENTITY_CREATION_FLAGS ecf, BLIT_ENTITY_CREATE_CONTEXT& context);

	// Called every frame to manage all Component systems
	void UpdateEntityComponents(RendererPtrType pRenderer, EntityManager* pManager, float deltaTime);

	// Called by UpdateEntityComponents to dispatch the assigned functions, for entities with game logic
	void UpdateGameLogic(RendererPtrType pRenderer, DynamicUpdateEntity* dynamicEntities, uint32_t dynamicEntityCount, float deltaTime);

	// Called before starting rendering to create the grids that will hold the collisions
	// This is done to avoid checking collisions for every object in the entire scene
	void PlaceCollisionsInGrid(Entity* entities, uint32_t entityCount, CollisionGrid* pGrids);

	void UpdateDynamicEntityGrid(Entity* entities, uint32_t entityCount, CollisionGrid* grids, uint32_t gridCount);

	void CheckCollisions(CollisionGrid* pGrid);

	void RotateEntity(Entity* pEntity, float deltaTime);
}