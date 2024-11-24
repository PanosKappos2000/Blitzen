#pragma once

#include "blitMemory.h"

#define BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER   2

namespace BlitCL
{
    template<typename T>
    class DynamicArray
    {
    public:

        DynamicArray(size_t initialSize =0)
            :m_size{sizeof(T) * initialSize}, m_capacity(sizeof(T) * initialSize * BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER)
        {
            m_pBlock = reinterpret_cast<T*>(BlitzenCore::BlitAlloc(BlitzenCore::AllocationType::DynamicArray, m_capacity));
        }

        inline size_t GetSize() { return m_size / sizeof(T); }

        inline T& operator [] (size_t index) { return m_pBlock[index]; }
        inline T& Front() { return m_pBlock[0]; }
        inline T& Back() { return m_pBlock[m_size - 1]; }
        inline T* Data() {return m_pBlock; }

        void Resize(size_t newSize)
        {
            // A different function will be used for downsizing
            if(newSize < m_size)
            {
                DEBUG_MESSAGE("DynamicArray::Resize(): New size %i is smaller than %i: \nUse Downsize(size_t) if this was intended", newSize, m_size)
                return;
            }
            // If the allocations have reached a point where the amount of elements is above the capacity, increase the capacity
            if(newSize > m_capacity)
            {
                RearrangeCapacity(newSize);
            }

            m_size = newSize * sizeof(T);
        }

        void Reserve(size_t size)
        {
            RearrangeCapacity(newSize / BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER);
        }

        void PushBack(T& newElement)
        {
            WARNING_MESSAGE("DynamicArray::PushBack called: This is not the best function for performance.\nConsider using Reserve() or Resize()")
            // If the allocations have reached a point where the amount of elements is above the capacity, increase the capacity
            if(m_size + sizeof(T) > m_capacity)
            {
                RearrangeCapacity(m_size + sizeof(T));
            }

            m_size += sizeof(T);
            m_pBlock[m_size] = newElement;
        }

        ~DynamicArray()
        {
            if(m_pBlock && m_size > sizeof(T))
            {
                BlitzenCore::BlitFree(BlitzenCore::AllocationType::DynamicArray, m_pBlock, m_size);
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
            m_capacity = sizeof(T) * newSize * BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER;
            T* pTemp = m_pBlock;
            m_pBlock = reinterpret_cast<T*>(BlitzenCore::BlitAlloc(BlitzenCore::AllocationType::DynamicArray, m_capacity));
            BlitzenCore::BlitMemCopy(m_pBlock, pTemp, m_capacity);
            BlitzenCore::BlitFree(BlitzenCore::AllocationType::DynamicArray, pTemp, temp);

            WARNING_MESSAGE("DynamicArray rearranged, this means that a memory allocation has taken place")
        }
    };
}