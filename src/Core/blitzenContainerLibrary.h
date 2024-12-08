#pragma once

#include "blitMemory.h"

#define BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER   2
#define BLIT_POINTER_TABLE_INITIAL_CAPACITY      5000

namespace BlitCL
{

    inline uint32_t Clamp(uint32_t initial, uint32_t upper, uint32_t lower) { 
        return (initial >= upper) ? upper /* return upper if the value is above it */ 
        : (initial <= lower) ? lower /* Else return lower if the value is below it*/ 
        : initial; /* Else just return the initial value */
    }



    template<typename T>
    class DynamicArray
    {
    public:

        DynamicArray(size_t initialSize = 0)
            :m_size{initialSize}, m_capacity(initialSize * BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER)
        {
            if (m_size > 0)
            {
                m_pBlock = reinterpret_cast<T*>(BlitzenCore::BlitAlloc(BlitzenCore::AllocationType::DynamicArray, m_capacity * sizeof(T)));
                BlitzenCore::BlitZeroMemory(m_pBlock, m_capacity * sizeof(T));
                return;
            }

            m_capacity = 0;
        }

        inline size_t GetSize() { return m_size; }

        inline T& operator [] (size_t index) { BLIT_ASSERT_DEBUG(index >= 0 && index < m_size) return m_pBlock[index]; }
        inline T& Front() { BLIT_ASSERT_DEBUG(m_size) m_pBlock[0]; }
        inline T& Back() { BLIT_ASSERT_DEBUG(m_size) return m_pBlock[m_size - 1]; }
        inline T* Data() {return m_pBlock; }

        void Resize(size_t newSize)
        {
            // A different function will be used for downsizing
            if(newSize < m_size)
            {
                BLIT_DBLOG("DynamicArray::Resize(): New size %i is smaller than %i: \nUse Downsize(size_t) if this was intended", newSize, m_size)
                return;
            }
            // If the allocations have reached a point where the amount of elements is above the capacity, increase the capacity
            if(newSize > m_capacity)
            {
                RearrangeCapacity(newSize);
                BlitzenCore::BlitZeroMemory(m_pBlock, (m_capacity * sizeof(T)) - (m_size * sizeof(T)));
            }

            m_size = newSize;
        }

        void Reserve(size_t size)
        {
            BLIT_ASSERT_DEBUG(size)
            RearrangeCapacity(size / BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER);
        }

        void PushBack(T& newElement)
        {
            BLIT_WARN("DynamicArray::PushBack called: This is not the best function for performance.\nConsider using Reserve() or Resize()")
            // If the allocations have reached a point where the amount of elements is above the capacity, increase the capacity
            if(m_size + 1 > m_capacity)
            {
                RearrangeCapacity(m_size + 1);
            }

            m_pBlock[m_size++] = newElement;
        }

        void RemoveAtIndex(size_t index)
        {
            if(index < m_size && index >= 0)
            {
                T* pTempBlock = m_pBlock;
                m_pBlock = reinterpret_cast<T*>(BlitzenCore::BlitAlloc(BlitzenCore::AllocationType::DynamicArray, m_capacity * sizeof(T)));
                BlitzenCore::BlitMemCopy(m_pBlock, pTempBlock, (index) * sizeof(T));
                BlitzenCore::BlitMemCopy(m_pBlock + index, pTempBlock + index + 1, (m_size - index) * sizeof(T));
                BlitzenCore::BlitFree(BlitzenCore::AllocationType::DynamicArray, pTempBlock, m_size * sizeof(T));
                m_size--;
            }
        }

        void Clear()
        {
            if(m_size)
            {
                BlitzenCore::BlitZeroMemory(m_pBlock, m_size * sizeof(T));
                m_size = 0;
            }
        }

        ~DynamicArray()
        {
            if(m_capacity > 0)
            {
                BlitzenCore::BlitFree(BlitzenCore::AllocationType::DynamicArray, m_pBlock, m_capacity * sizeof(T));
            }
        }

    private:

        size_t m_size;
        size_t m_capacity;
        T* m_pBlock;

    private:

        void RearrangeCapacity(size_t newSize)
        {
            size_t temp = m_capacity;
            m_capacity = newSize * BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER;
            T* pTemp = m_pBlock;
            m_pBlock = reinterpret_cast<T*>(BlitzenCore::BlitAlloc(BlitzenCore::AllocationType::DynamicArray, m_capacity * sizeof(T)));
            if (m_size != 0)
            {
                BlitzenCore::BlitMemCopy(m_pBlock, pTemp, m_capacity * sizeof(T));
            }
            if(temp != 0)
                BlitzenCore::BlitFree(BlitzenCore::AllocationType::DynamicArray, pTemp, temp * sizeof(T));

            BLIT_WARN("DynamicArray rearranged, this means that a memory allocation has taken place")
        }
    };



    /*------------------------------------------------------------------------------------------
        This structure is a very simple hash table that only takes pointers.
        It can never own any memory, so the objects it point to should be managed outside.
        It also only accepts names for keys. Better hash tables will be created at a later time
    ---------------------------------------------------------------------------------------------*/
    template<typename T>
    class PointerTable
    {
    public:

        PointerTable()
            :m_capacity(BLIT_POINTER_TABLE_INITIAL_CAPACITY)
        {
            if(m_capacity > 0)
            {
                m_pBlock = reinterpret_cast<T**>(BlitzenCore::BlitAllocLinear(BlitzenCore::AllocationType::Hashmap, sizeof(T*) * m_capacity));
                BlitzenCore::BlitZeroMemory(m_pBlock, sizeof(T*) * m_capacity);
            }
        }

        void Set(const char* name, T* pValue)
        {
            size_t hash = Hash(name);

            m_pBlock[hash] = pValue ? pValue : nullptr;
        }

        T* Get(const char* name, T* pDefault)
        {
            size_t  hash = Hash(name);
            
            return m_pBlock[hash] ? m_pBlock[hash] : pDefault;
        }

        ~PointerTable()
        {
            if(m_capacity > 0)
            {
                BlitzenCore::BlitZeroMemory(m_pBlock, sizeof(T**) * m_capacity);
            }
        }

    private:

        size_t m_capacity;
        // Since this can only take pointers, the memory block will be a pointer to pointers
        T** m_pBlock;

    private:

        size_t Hash(const char* name)
        {
            // A multipler to use when generating a hash. Prime to hopefully avoid collisions
            static const size_t multiplier = 97;
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
    };
}