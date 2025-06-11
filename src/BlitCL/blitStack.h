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

		DATA* Get()
		{
			return m_data;
		}

		DATA& operator [](uint32_t idx)
		{
			return m_data[idx];
		}

	private:
		uint32_t m_count{ 0 };
		DATA m_data[MAX_COUNT];
	};

	template<class DATA, uint32_t MAX_COUNT>
	class StaticAllocator
	{
		StaticAllocator()
		{
			uint32_t idx = MAX_COUNT - 1;
			for (auto& data : m_idxs)
			{
				data = idx--;
			}
		}

		uint32_t Add(DATA& newcomer)
		{
			uint32_t idx = m_idxs.Pop();
			m_data[idx] = newcomer;
		}

		uint32_t Copy(DATA* ptr)
		{
			uint32_t idx = m_idxs.Pop();
			BlitzenCore::BlitMemCopy<DATA>(&m_data[idx], ptr, sizeof(DATA));
		}

		DATA* GetNext()
		{

		}

		void Remove(uint32_t idx)
		{
			
		}

		uint32_t GetCount() const
		{
			return m_data.m_count();
		}

		uint32_t GetSize() const
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
		DATA m_data[MAX_COUNT];
		uint32_t m_count{ 0 };

		BlitStack<uint32_t, MAX_COUNT> m_idxs;
	};
}