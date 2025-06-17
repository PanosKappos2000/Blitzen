#pragma once 

#include "blitStack.h"

namespace BlitCL
{
	class BLIT_L_ALLOC
	{
		inline BLIT_L_ALLOC(size_t size) 
		{
			m_pData = BlitzenCore::BlitAlloc<uint8_t>(BlitzenCore::AllocationType::LinearAlloc, size);
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

		~BLIT_L_ALLOC()
		{

		}

	private:
		void* m_pData;
		size_t m_size;
		size_t m_point{ 0 };
	};

}