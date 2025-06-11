#pragma once

#include "Core/blitMemory.h"

namespace BlitCL
{
	template<class DATA, uint32_t MAX_COUNT>
	class BlitStack
	{
	public:

		inline BlitStack(uint32_t count = 0) : m_count{count}
		{
			
		}

		void Add(DATA data)
		{
			m_data[m_count++] = data;
		}

		DATA& Pop()
		{
			return m_data[--m_count];
		}

		bool IsEmpty() const
		{
			return m_count == 0;
		}

		bool IsFull() const
		{
			return m_count == MAX_COUNT;
		}

		uint32_t Count() const
		{
			return m_count;
		}

		uint32_t Size() const
		{
			return MAX_COUNT;
		}

		DATA* Get() const
		{
			return m_data;
		}

		DATA& operator [](uint32_t idx) const
		{
			return m_data[idx];
		}

	private:
		uint32_t m_count{ 0 };
		DATA m_data[MAX_COUNT];
	};

	template<class DATA, uint32_t MAX_COUNT>
	class BLIT_S_ALLOC
	{
		BLIT_S_ALLOC(uint32_t count = 0) : m_stack{ count }
		{

		}

		uint32_t ADD(DATA& newcomer)
		{
			BLIT_ASSERT_MESSAGE(!m_stack.IsFull(), "Exceeded stack allocator count");

			auto pNewcomer{ &pManager->m_dynamicTransforms[idx] };

			if (m_idxs.IsEmpty())
			{
				m_stack.Add(newcomer);
			}

			uint32_t idx = m_idxs.Pop;
			pManager->m_movingEntities[idx] = pNewcomer;
		}

		uint32_t Count() const
		{
			return m_count;
		}

		uint32_t Size() const
		{
			return MAX_COUNT;
		}

		DATA* Get() const
		{
			return m_data;
		}

		DATA& operator [](uint32_t idx) const
		{
			return m_data[idx];
		}


	private:
		BlitStack<uint32_t, MAX_COUNT> m_idxs[MAX_COUNT];
		BlitStack<DATA, MAX_COUNT> m_stack;
	};
}