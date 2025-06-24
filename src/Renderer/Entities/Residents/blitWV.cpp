#include "blitWV.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"
#include "WorldVariables/WVRotatingKitten.h"

namespace BlitzenEngine
{
	inline static WVHOST* wv_pHost_STATIC_ACCESS{ nullptr };

	static WVHANDLE GetWorldVariable(WVKEY key, const WVHOST* wv_pHost)
	{
		auto& desc{ wv_pHost->m_descs[key.wv_type.id] };
		return &reinterpret_cast<uint8_t*>(wv_pHost->m_pPool)[desc.m_offset + key.wv_inst.inst * desc.m_typeSize];
	}

	void WorldVariableStart(WVHANDLE wv, WVTYPE type)
	{
		switch (type.id)
		{
		case 0:
		{
			// reinterpret_cast<WVRotatingKitten*>(wv)->Start();
			break;
		}
		}
	}

	void WorldVariableTick(WVKEY key, const WVHOST& host)
	{
		switch (key.wv_type.id)
		{
		case 0:
		{
			reinterpret_cast<WVRotatingKitten*>(GetWorldVariable(key, &host))->Tick();
			break;
		}
		}
	}

	void WorldVariableCollision(WVHANDLE sender, WVTYPE senderType, WVHANDLE receiver, WVTYPE receiverID)
	{
		switch (senderType.id)
		{
		case 0:
		{
			// reinterpret_cast<WVRotatingKitten*>(wv)->Collision(receiver, receiverID);
			break;
		}
		}
	}

	void AllocateWorldVariables_STATIC_ACCESS(uint32_t poolSize)
	{
		uint32_t biggestWV{ 0 };
		for (uint32_t dsc = 0; dsc < WVTYPES; ++dsc)
		{
			biggestWV = wv_pHost_STATIC_ACCESS->m_descs[dsc].m_typeSize > biggestWV ? wv_pHost_STATIC_ACCESS->m_descs[dsc].m_typeSize : biggestWV;
		}

		if (biggestWV == 0)
		{
			biggestWV = 1;
		}

		wv_pHost_STATIC_ACCESS->m_pPool = BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::WV, poolSize * biggestWV);
		wv_pHost_STATIC_ACCESS->m_poolSize = poolSize * biggestWV;
	}

	void AddClientWorldVariableDescriptions()
	{
		
	}

	void AddWorldVariable_STATIC_ACCESS(WVKEY* pwv)
	{
		pwv->wv_inst.inst = wv_pHost_STATIC_ACCESS->m_descs[pwv->wv_type.id].NEW();
	}

	WVHANDLE GetWorldVariable_STATIC_ACCESS(WVKEY key)
	{
		return GetWorldVariable(key, wv_pHost_STATIC_ACCESS);
	}

	void WVDESC::OFFSET(uint32_t maxInstances, uint32_t size, WVTYPE typeID)
	{
		m_maxInstances = maxInstances;
		m_typeSize = size;
		m_wv_type = typeID;
		m_instanceCount = 0;
		m_offset = size * maxInstances;
	}

	uint32_t WVDESC::NEW()
	{
		if (m_instanceCount >= m_maxInstances)
		{
			BLIT_ERROR("Exceeded maximum instance count for world variable with flag: %u", m_wv_type.id);
			BLIT_ASSERT(true);
		}

		return m_instanceCount++;
	}

	void WVHOST::AddClientWorldVariableDescriptions()
	{
		m_descs[0].m_instanceCount = 0;
		m_descs[0].m_maxInstances = 5'000;
		m_descs[0].m_offset = 0;
		m_descs[0].m_wv_type.id = 0;
		m_descs[0].m_typeSize = sizeof(WVRotatingKitten);
	}

	WVHOST::~WVHOST()
	{
		if (m_pPool)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::WV, m_pPool, m_poolSize);
		}
	}

	void InitializeWorldVariableContextPtr_STATIC_ACCESS(WVHOST* ptr)
	{
		BLIT_ASSERT_MESSAGE(wv_pHost_STATIC_ACCESS == nullptr, "Tried to reinitialize world variable context pointer");

		wv_pHost_STATIC_ACCESS = ptr;
	}
}