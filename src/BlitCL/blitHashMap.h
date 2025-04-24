#pragma once
#include "Core/blitMemory.h"

namespace BlitCL
{
    template<typename T>
    class HashMap
    {
    public:

        HashMap(size_t initialCapacity = 10):
            m_capacity{ initialCapacity }, 
            m_size{ 0 }, 
            m_pBlock{BlitzenCore::BlitAlloc<T>(MapAlloc, m_capacity)}
        {}

        ~HashMap()
        {
            // Calls destructors for elements up to m_size.
            for (size_t i = 0; i < m_size; ++i)
            {
                m_pBlock[i].~T(); 
            }
            // Free the entire memory block.
            BlitzenCore::BlitFree<T>(MapAlloc, m_pBlock, m_capacity);
        }

        void Insert(const char* name, const T& value)
        {
            if (m_capacity <= m_size + 1)
            {
                IncreaseCapacity(m_capacity + 1);
            }

            m_pBlock[HashFunction(name)] = value;
            ++m_size;
        }

        inline const T& operator [](const char* name) { return m_pBlock[HashFunction(name)]; }
        inline T& GetVar(const char* name) { return m_pBlock[HashFunction(name)]; }

    private:

        size_t m_capacity;
        size_t m_size;

        T* m_pBlock;

    private:

        size_t HashFunction(const char* name)
        {
            // A multipler to use when generating a hash. Prime to hopefully avoid collisions
            constexpr size_t multiplier = 97;
            unsigned const char* us;

            size_t hash = 0;
            for (us = (unsigned const char*)name; *us; us++)
            {
                hash = hash * multiplier + *us;
            }

            // Mod it against the size of the table.
            hash %= m_capacity;

            return hash;
        }

    private:
        void IncreaseCapacity(size_t newSize)
        {
            size_t temp = m_capacity;
            m_capacity = newSize * ce_blitDynamiArrayCapacityMultiplier;

            T* pTemp = m_pBlock;
            m_pBlock = BlitzenCore::BlitAlloc<T>(MapAlloc, m_capacity);

            if (temp != 0)
            {
                BlitzenCore::BlitMemCopy(m_pBlock, pTemp, temp * sizeof(T));
                BlitzenCore::BlitFree<T>(MapAlloc, pTemp, temp);
            }
        }
    };
}