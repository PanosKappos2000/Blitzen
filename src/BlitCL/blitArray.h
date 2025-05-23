#pragma once
#include "Core/blitMemory.h"
#include "blitArrayIterator.h"

#define BLIT_ARRAY_SIZE(array)   sizeof(array) / sizeof(array[0])

namespace BlitCL
{
    template<class T, size_t SIZE>
    class StaticArray
    {
    public:
        StaticArray()
        {
            static_assert(SIZE > 0);
        }

        StaticArray(const T& data)
        {
            static_assert(SIZE > 0);

            for (size_t i = 0; i < SIZE; ++i)
            {
                BlitzenCore::BlitMemCopy(&m_array[i], &data, sizeof(T));
            }
        }

        StaticArray(const StaticArray<T, SIZE>& array) = delete;
		StaticArray operator = (const StaticArray<T, SIZE>& array) = delete;

        StaticArray(T&& data)
        {
            static_assert(SIZE > 0);

            for (size_t i = 0; i < SIZE; ++i)
            {
                BlitzenCore::BlitMemCopy(&m_array[i], &data, sizeof(T));
            }
        }

        using Iterator = DynamicArrayIterator<T>;
        Iterator begin() { return Iterator(m_array); }
        Iterator end() { return Iterator(m_array + SIZE); }

        inline T& operator [] (size_t idx) { return m_array[idx]; }

        inline T* Data() { return m_array; }

        inline size_t Size() { return S; }

        ~StaticArray()
        {
        
        }
    private:
        T m_array[SIZE];
    };
}