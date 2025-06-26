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

	void WorldVariableTick(WVKEY key, const WVHOST& host, float deltaTime)
	{
		switch (key.wv_type.id)
		{
		case 0:
		{
			reinterpret_cast<WVRotatingKitten*>(GetWorldVariable(key, &host))->Tick(deltaTime);
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
			BLIT_ASSERT(false);
		}

		return m_instanceCount++;
	}

	void WVHOST::AddClientWorldVariableDescriptions(const WV_CONTEXT& wvContext, WVDESC* wvDescriptions, uint32_t uniqueWvCount)
	{
		if (wvContext.m_wvTypeCount > CE_MAX_WORLD_WV_UNIQUE_TYPES)
		{
			BLIT_ERROR("%s: Exceeded maximum world variable unique types (%u > %u)", BlitzenCore::CE_WORLD_VARIABLE_SYSTEM_NAME, wvContext.m_wvTypeCount, CE_MAX_WORLD_WV_UNIQUE_TYPES);
			BLIT_ASSERT(false);
		}

		uint32_t biggestWV = 0;
		for (uint32_t desc = 0; desc < uniqueWvCount; ++desc)
		{
			auto& write{ m_descs[desc] };
			auto& read{ wvDescriptions[desc] };

			if (read.m_wv_type.id != desc)
			{
				BLIT_ERROR("%s: World Variable with type ID: %u, violated rule: ", BlitzenCore::CE_WORLD_VARIABLE_SYSTEM_NAME, read.m_wv_type.id);
				BLIT_INFO("%s: World Variable descriptions must be written in ascending order", BlitzenCore::CE_WORLD_VARIABLE_SYSTEM_NAME);
				BLIT_ASSERT(false);
			}

			write.m_instanceCount = read.m_instanceCount;
			write.m_maxInstances = read.m_maxInstances;
			write.m_wv_type.id = desc;
			write.m_typeSize = read.m_typeSize;
			write.m_offset = m_currentPoolOffset;

			m_currentPoolOffset += read.m_typeSize * read.m_maxInstances;

			biggestWV = read.m_typeSize > biggestWV ? read.m_typeSize : biggestWV;
		}

		m_poolSize = biggestWV * m_currentPoolOffset;
		wv_pHost_STATIC_ACCESS->m_pPool = BlitzenCore::MANUAL_ALLOC(BlitzenCore::AllocationType::WV, m_poolSize);
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

	void AddWorldVariable_STATIC_ACCESS(WVKEY* pwv)
	{
		pwv->wv_inst.inst = wv_pHost_STATIC_ACCESS->m_descs[pwv->wv_type.id].NEW();
	}

	WVHANDLE GetWorldVariable_STATIC_ACCESS(WVKEY key)
	{
		return GetWorldVariable(key, wv_pHost_STATIC_ACCESS);
	}
}