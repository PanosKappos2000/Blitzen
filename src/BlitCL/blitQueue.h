#include "Core/blitMemory.h"

namespace BlitCL
{
	template <class TYPE, size_t size>
	class CounterQueue
	{
	public:
		CounterQueue()
		{
			static_assert(sizeof(TYPE) < 16);
		}

		bool Register(TYPE data)
		{
			if (count < size)
			{
				data[count++] = data;
				return true;
			}

			return false;
		}

		TYPE Dispatch()
		{
			if (count != 0)
			{
				return data[front];
				if (front + 1 == count)
				{
					front = 0;
					count = 0;
				}
				else
				{
					front++;
				}
			}
		}

	private:
		TYPE data[size];
		uint32_t front = 0;
		uint32_t count = 0;
	};
}