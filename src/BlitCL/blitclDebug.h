#pragma once 
#include "blitzenContainerLibrary.h"

namespace BlitCL
{
	template<class DATA>
	void LogContainerData(DynamicArray<DATA>& item, const char* itemName)
	{
#if !defined(NDEBUG)
		for (const auto& data : item)
		{
			BLIT_DBLOG("%s->Data: %s", itemName, data.DBLOG());
		}
#endif
	}

	template<typename T>
	void LogContainerPrimitiveData(DynamicArray<T>& item, const char* itemName, const char* stringArg)
	{
#if !defined(NDEBUG)
		for (const auto& p : item)
		{
			BLIT_DBLOG("{%s DATA}: %s", itemName, stringArg, p);
		}
	}
#endif
}