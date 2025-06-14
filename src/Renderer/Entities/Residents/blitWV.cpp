#include "blitWV.h"
#include "Client/ClientWV/wvData.h"

namespace BlitzenEngine
{
	constexpr uint32_t WVTYPES = 1;
	constexpr WVTYPEID WVRotatingKittenType = 0;

	inline static WORLD_VARIABLE_CONTEXT S_WORLD_VARIABLE_CONTEXT_ARRAY[WVTYPES]
	{
		{1'000, sizeof(WVRotatingKitten)}
	};

	void WorldVariableStart(WVHANDLE wv, WVTYPEID type)
	{
		switch (type)
		{
		case WVRotatingKittenType:
		{
			// reinterpret_cast<WVRotatingKitten*>(wv)->Create();
			break;
		}
		}
	}

	void WorldVariableTick(WVHANDLE wv, WVTYPEID type)
	{
		switch (type)
		{
		case WVRotatingKittenType:
		{
			// reinterpret_cast<WVRotatingKitten*>(wv)->Tick();
			break;
		}
		}
	}

	void WorldVariableCollision(WVHANDLE sender, WVTYPEID senderType, WVHANDLE receiver, WVTYPEID receiverID)
	{
		switch (senderType)
		{
		case WVRotatingKittenType:
		{
			// reinterpret_cast<WVRotatingKitten*>(wv)->Collision(receiver, receiverID);
			break;
		}
		}
	}

	void AllocateWorldVariables(uint32_t count, WVHOST* pHost)
	{
		for (WVTYPEID flag = 0; flag < count; ++flag)
		{
			pHost->ALLOC(flag);
		}
	}

	void AddWorldVariable(WV* pwv, WVHOST* pHost)
	{
		pHost->ADD(pwv);
	}

	void WVGROUP::ALLOC(WVTYPEID typeID)
	{
		auto& context{ S_WORLD_VARIABLE_CONTEXT_ARRAY[typeID] };

		m_pool = BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::WV, context.m_size * context.m_maxInstances);
		m_size = context.m_size;
		m_type = typeID;
	}

	void WVGROUP::ADD(WV* pWV)
	{
		if (m_instanceCount >= S_WORLD_VARIABLE_CONTEXT_ARRAY[m_type].m_maxInstances)
		{
			BLIT_ERROR("Exceeded maximum instance count for world variable with flag: %u", m_type);
			BLIT_ASSERT(true);
		}

		pWV->m_handle = &reinterpret_cast<uint8_t*>(m_pool)[m_size * m_instanceCount];
		pWV->m_type = m_type;
		pWV->m_instance = m_instanceCount++;
	}

	WVGROUP::~WVGROUP()
	{
		BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::WV, m_pool, m_size * S_WORLD_VARIABLE_CONTEXT_ARRAY[m_type].m_maxInstances);
	}
}