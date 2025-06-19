#pragma once
#include "Core/blitMemory.h"

namespace BlitzenEngine
{
	using WVHANDLE = void*;

	struct WVTYPE
	{
		uint32_t id;
	};
	
	using WVSPOTS = uint32_t*;
	
	struct WVINST
	{
		uint32_t inst;
	};

	class WORLD_VARIABLE_CONTEXT
	{
	public:

		uint32_t m_maxInstances;
		uint32_t m_instanceCount;
		uint32_t m_typeSize;
		WVTYPE m_wv_type;
		void* m_pPool;

		WORLD_VARIABLE_CONTEXT(uint32_t maxInstances, uint32_t size, WVTYPE typeID)
			:m_maxInstances{ maxInstances }, m_typeSize(size), m_wv_type{ typeID }, m_instanceCount{ 0 }, m_pPool{ nullptr }
		{

		}

		void ALLOC();

		uint32_t NEW();

		WVHANDLE GET(WVINST wv_inst);

		~WORLD_VARIABLE_CONTEXT();
	};

	struct WVKEY
	{
		WVTYPE wv_type;
		WVINST wv_inst;
	};

	void InitializeWorldVariableContextPtr_STATIC_ACCESS(WORLD_VARIABLE_CONTEXT* ptr);

	void AllocateWorldVariables_STATIC_ACCESS(uint32_t count);

	void AddWorldVariable_STATIC_ACCESS(WVKEY* pwv);

	WVHANDLE GetWorldVariable_STATIC_ACCESS(WVKEY pwv);

	void WorldVariableStart(WVHANDLE handle, WVTYPE type);

	void WorldVariableTick(WVHANDLE handle, WVTYPE type);

	void WorldVariableCollision(WVHANDLE sender, WVTYPE senderType, WVHANDLE receiver, WVTYPE receiverType);
}