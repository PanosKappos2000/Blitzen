#include "blitWV.h"
#include "Client/ClientWV/wvData.h"
#include "Core/DbLog/blitLogger.h"
#include "Core/DbLog/blitAssert.h"

namespace BlitzenEngine
{
	constexpr uint32_t WVTYPES = 1;
	constexpr uint32_t WVRotatingKitten_ContextID = 0;

	inline static WORLD_VARIABLE_CONTEXT* wvCtxArr{ nullptr };
	/*{
		{1'000, sizeof(WVRotatingKitten), 0}
	};*/

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

	void AllocateWorldVariables_STATIC_ACCESS(uint32_t count)
	{
		if (count >= BlitzenCore::Ce_MaxWorldVariableCount)
		{
			BLIT_ERROR("%s: Too many world variables requested", BlitzenCore::CE_WORLD_VARIABLE_SYSTEM_NAME);
			return;
		}

		for (uint32_t id = 0; id < count; ++id)
		{
			wvCtxArr[id].ALLOC();
		}
	}

	void AddWorldVariable_STATIC_ACCESS(WVKEY* pwv)
	{
		pwv->wv_inst.inst = wvCtxArr[pwv->wv_type.id].NEW();
	}

	WVHANDLE GetWorldVariable_STATIC_ACCESS(WVKEY pwv)
	{
		return wvCtxArr[pwv.wv_type.id].GET(pwv.wv_inst);
	}

	void WORLD_VARIABLE_CONTEXT::ALLOC()
	{
		m_pPool = BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::WV, m_typeSize * m_maxInstances);
	}

	uint32_t WORLD_VARIABLE_CONTEXT::NEW()
	{
		if (m_instanceCount >= m_maxInstances)
		{
			BLIT_ERROR("Exceeded maximum instance count for world variable with flag: %u", m_wv_type.id);
			BLIT_ASSERT(true);
		}

		return m_instanceCount++;
	}

	WVHANDLE WORLD_VARIABLE_CONTEXT::GET(WVINST wv_inst)
	{
		return &reinterpret_cast<uint8_t*>(m_pPool)[m_typeSize * wv_inst.inst];
	}

	WORLD_VARIABLE_CONTEXT::~WORLD_VARIABLE_CONTEXT()
	{
		if (m_pPool)
		{
			BlitzenCore::MANUAL_FREE(BlitzenCore::AllocationType::WV, m_pPool, m_typeSize * m_maxInstances);
		}
	}

	void InitializeWorldVariableContextPtr_STATIC_ACCESS(WORLD_VARIABLE_CONTEXT* ptr)
	{
		BLIT_ASSERT_MESSAGE(wvCtxArr == nullptr, "Tried to reinitialize world variable context pointer");

		wvCtxArr = ptr;
	}
}