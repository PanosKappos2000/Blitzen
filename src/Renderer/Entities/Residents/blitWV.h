#pragma once
#include "Core/blitMemory.h"

namespace BlitzenEngine
{
	using WVHANDLE = void*;
	using WVTYPEID = uint32_t;
	using WVMAX = uint32_t;
	using WVSPOTS = uint32_t*;
	using WVSIZE = uint32_t;
	using WVINST = uint32_t;

	struct WORLD_VARIABLE_CONTEXT
	{
		WVMAX m_maxInstances;
		WVSIZE m_size;

		WORLD_VARIABLE_CONTEXT(WVMAX maxInstances, WVSIZE size) :m_maxInstances{ maxInstances }, m_size(size)
		{

		}
	};

	struct WV
	{
		WVHANDLE m_handle;
		WVTYPEID m_type;
		WVINST m_instance;
	};

	class WVGROUP
	{
	public:
		void ALLOC(WVTYPEID typeID);
		
		void ADD(WV* pWV);

		// void REMOVE

		~WVGROUP();

	private:
		WVHANDLE m_pool;
		WVSIZE m_size;
		WVSPOTS m_spots;
		WVTYPEID m_type;

		uint32_t m_instanceCount{ 0 };
	};

	class WVHOST
	{
	public:
		void ALLOC(WVTYPEID flag);

		void ADD(WV* pWV);

	private:
		WVGROUP m_groups[BlitzenCore::Ce_MaxWorldVariableCount]{};
		uint32_t m_groupCount{ 0 };
	};

	void AllocateWorldVariables(uint32_t count, WVHOST* pHost);

	void AddWorldVariable(WV* pwv, WVHOST* pHost);

	void WorldVariableStart(WVHANDLE handle, WVTYPEID type);

	void WorldVariableTick(WVHANDLE handle, WVTYPEID type);

	void WorldVariableCollision(WVHANDLE sender, WVTYPEID senderType, WVHANDLE receiver, WVTYPEID receiverType);
}