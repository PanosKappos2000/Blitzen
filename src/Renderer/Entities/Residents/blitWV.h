#pragma once
#include "Core/blitMemory.h"

namespace BlitzenEngine
{
	using WVHANDLE = void*;
	constexpr uint32_t WVTYPES = 1;

	struct WVTYPE
	{
		uint32_t id;
	};
	
	using WVSPOTS = uint32_t*;
	
	struct WVINST
	{
		uint32_t inst;
	};

	class WVDESC
	{
	public:

		uint32_t m_maxInstances{ 0 };
		uint32_t m_instanceCount{ 0 };
		uint32_t m_typeSize{ 0 };
		WVTYPE m_wv_type{ 0 };
		uint32_t m_offset{ 0 };

		void OFFSET(uint32_t maxInstances, uint32_t size, WVTYPE typeID);

		uint32_t NEW();
	};

	struct WVKEY
	{
		WVTYPE wv_type;
		WVINST wv_inst;
	};

	class WVHOST
	{
	public:

		void* m_pPool{ nullptr };
		WVDESC m_descs[WVTYPES];
		uint32_t m_poolSize{ 0 };

		~WVHOST();
	};

	void InitializeWorldVariableContextPtr_STATIC_ACCESS(WVHOST* ptr);

	void AllocateWorldVariables_STATIC_ACCESS(uint32_t poolSize);

	void AddWorldVariable_STATIC_ACCESS(WVKEY* pwv);

	WVHANDLE GetWorldVariable_STATIC_ACCESS(WVKEY pwv);

	void WorldVariableStart(WVHANDLE handle, WVTYPE type);

	void WorldVariableTick(WVHANDLE handle, WVTYPE type);

	void WorldVariableCollision(WVHANDLE sender, WVTYPE senderType, WVHANDLE receiver, WVTYPE receiverType);
}