#pragma once 

#include "blitStack.h"

namespace BlitCL
{
	
	template<size_t SIZE>
	class BLIT_L_ALLOC
	{
		inline BLIT_ST_ALLOC() 
		{
			m_pData = BlitzenCore::BlitAlloc<uint8_t>(BlitzenCore::AllocationType::LinearAlloc, SIZE);
		}

		template<class DATA>
		void* Get()
		{
			BLIT_ASSERT_MESSAGE(m_point + sizeof(DATA) <= SIZE, "Exceeded linear allocator");

			void* res{ &m_pData[m_point] };
			m_point += sizeof(DATA);
			return res;
		}

		void Reset()
		{

		}

		~BLIT_ST_ALLOC()
		{

		}

	private:
		void* m_pData;
		size_t m_size;
		size_t m_point{ 0 };
	};

}