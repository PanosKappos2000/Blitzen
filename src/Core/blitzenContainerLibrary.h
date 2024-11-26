#pragma once

#include "blitMemory.h"

#define BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER   2

namespace BlitCL
{
    template<typename T>
    class DynamicArray
    {
    public:

        DynamicArray(size_t initialSize = 0)
            :m_size{initialSize}, m_capacity(initialSize * BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER)
        {
            if (m_size)
            {
                m_pBlock = reinterpret_cast<T*>(BlitzenCore::BlitAlloc(BlitzenCore::AllocationType::DynamicArray, m_capacity * sizeof(T)));
                BlitzenCore::BlitZeroMemory(m_pBlock, m_capacity * sizeof(T));
            }
        }

        inline size_t GetSize() { return m_size; }

        inline T& operator [] (size_t index) { return m_pBlock[index]; }
        inline T& Front() { return m_pBlock[0]; }
        inline T& Back() { return m_pBlock[m_size - 1]; }
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
            if(m_pBlock && m_capacity > 0)
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
}