#pragma once
#include "Core/blitMemory.h"

namespace BlitzenEngine
{
	using WVHANDLE = void*;
	constexpr uint32_t CE_MAX_WORLD_WV_UNIQUE_TYPES = 1;

	// World Variable Identifiers
	struct WVTYPE
	{
		uint32_t id;
	};
	struct WVINST
	{
		uint32_t inst;
	};

	// Single wv type description
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

	// Key to the wv pool for an individual world variable instance
	struct WVKEY
	{
		WVTYPE wv_type;
		WVINST wv_inst;
	};

	// Full context of client world variables
	struct WV_CONTEXT
	{
		uint32_t m_wvTypeCount{ 0 };
	};

	// World Variable Pool Manager
	class WVHOST
	{
	public:

		void* m_pPool{ nullptr };
		uint32_t m_poolSize{ 0 };
		uint32_t m_currentPoolOffset{ 0 };
		WVDESC m_descs[CE_MAX_WORLD_WV_UNIQUE_TYPES];

		void AddClientWorldVariableDescriptions(const WV_CONTEXT& wvContext, WVDESC* wvDescriptions, uint32_t uniqueWvCount);

		~WVHOST();
	};

	void InitializeWorldVariableContextPtr_STATIC_ACCESS(WVHOST* ptr);

	void AddWorldVariable_STATIC_ACCESS(WVKEY* pwv);

	WVHANDLE GetWorldVariable_STATIC_ACCESS(WVKEY pwv);

	void WorldVariableStart(WVKEY key, uint32_t residentID);

	void WorldVariableTick(WVKEY key, const WVHOST& host, float deltaTime);

	void WorldVariableCollision(WVHANDLE sender, WVTYPE senderType, WVHANDLE receiver, WVTYPE receiverType);
}