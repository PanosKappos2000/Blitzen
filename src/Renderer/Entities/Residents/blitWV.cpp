#include "blitWV.h"
#include "Client/ClientWV/wvData.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
	constexpr uint32_t WVRotatingKitten_ContextID = 0;

	inline static WVHOST* p_wv_host{ nullptr };

	void WorldVariableStart(WVHANDLE wv, WVTYPE type)
	{
		switch (type.id)
		{
		case WVRotatingKitten_ContextID:
		{
			// reinterpret_cast<WVRotatingKitten*>(wv)->Create();
			break;
		}
		}
	}

	void WorldVariableTick(WVHANDLE wv, WVTYPE type)
	{
		switch (type.id)
		{
		case 0:
		{
			// reinterpret_cast<WVRotatingKitten*>(wv)->Tick();
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
		p_wv_host->m_pPool = BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::WV, poolSize);
		p_wv_host->m_poolSize = poolSize;
	}

	void AddWorldVariable_STATIC_ACCESS(WVKEY* pwv)
	{
		pwv->wv_inst.inst = p_wv_host->m_descs[pwv->wv_type.id].NEW();
	}

	WVHANDLE GetWorldVariable_STATIC_ACCESS(WVKEY key)
	{
		auto& desc{ p_wv_host->m_descs[key.wv_type.id] };
		return &reinterpret_cast<uint8_t*>(p_wv_host->m_pPool)[desc.m_offset + key.wv_inst.inst * desc.m_typeSize] ;
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

	WVHOST::~WVHOST()
	{
		if (m_pPool)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::WV, m_pPool, m_poolSize);
		}
	}

	void InitializeWorldVariableContextPtr_STATIC_ACCESS(WVHOST* ptr)
	{
		BLIT_ASSERT_MESSAGE(p_wv_host == nullptr, "Tried to reinitialize world variable context pointer");

		p_wv_host = ptr;
	}
}