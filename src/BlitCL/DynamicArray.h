#pragma once
#include "Core/blitMemory.h"
#include "blitArrayIterator.h"

namespace BlitCL
{
    // Warning this class is way more dangerous than std::vector. 
    // It initializes all memory with malloc and expects the user to provide data for each struct when they wish to do so.
    template<typename T>
    class DynamicArray
    {
    public:

        DynamicArray()
            :m_size{ 0 }, m_capacity{ 0 }, m_pBlock{ nullptr }
        {
        }

        // Allocates memory for initialSize elements. NO CONSTRUCTORS CALLED!
        DynamicArray(size_t initialSize)
            :m_size{ initialSize },
            m_capacity{ initialSize * ce_blitDynamiArrayCapacityMultiplier },
            m_pBlock{ BlitzenCore::BlitAlloc<T>(DArrayAlloc, m_capacity) }
        {
        }

        // Allocates memory for initialSize elements. Copies the data to every element up to m_size
        DynamicArray(size_t initialSize, const T& data)
            :m_size{ initialSize },
            m_capacity{ initialSize * ce_blitDynamiArrayCapacityMultiplier },
            m_pBlock{ BlitzenCore::BlitAlloc<T>(DArrayAlloc, m_capacity) }
        {
            for (size_t i = 0; i < initialSize; ++i)
                BlitzenCore::BlitMemCopy(&m_pBlock[i], &data, sizeof(T));
        }

        // Allocates memory for initialSize elements. Copies the data to every element up to m_size
        DynamicArray(size_t initialSize, T&& data)
            :m_size{ initialSize },
            m_capacity{ initialSize * ce_blitDynamiArrayCapacityMultiplier },
            m_pBlock{ BlitzenCore::BlitAlloc<T>(DArrayAlloc, m_capacity) }
        {
            for (size_t i = 0; i < initialSize; ++i)
                BlitzenCore::BlitMemCopy(&m_pBlock[i], &data, sizeof(T));
        }

        DynamicArray(DynamicArray<T>& array)
            :m_size{ array.GetSize() },
            m_capacity{ array.GetSize() * ce_blitDynamiArrayCapacityMultiplier }, 
            m_pBlock{ BlitzenCore::BlitAlloc<T>(DArrayAlloc, m_capacity) }
        {
            BlitzenCore::BlitMemCopy(m_pBlock, array.Data(), array.GetSize() * sizeof(T));
        }

        DynamicArray(const DynamicArray<T>& copy) = delete;
        DynamicArray<T> operator = (const DynamicArray<T>& copy) = delete;

        ~DynamicArray()
        {
            if (m_capacity > 0)
            {
                // Calls destructors for elements up to m_size.
                for (size_t i = 0; i < m_size; ++i)
                {
                    m_pBlock[i].~T(); 
                }
                // Free the entire memory block.
                BlitzenCore::BlitFree<T>(DArrayAlloc, m_pBlock, m_capacity);
            }
        }


        using Iterator = DynamicArrayIterator<T>;
        inline Iterator begin() const { return Iterator(m_pBlock); }
        inline Iterator end() const { return Iterator(m_pBlock + m_size); }
        inline Iterator cbegin() const { return Iterator(m_pBlock); }
        inline Iterator cend() const { return Iterator(m_pBlock + m_size); }

        inline size_t GetSize() const { return m_size; }
        inline T& operator [] (size_t index) const{ return m_pBlock[index]; }
        inline T& At(size_t index) const 
        { 
            BLIT_ASSERT(index < m_size); 
            return m_pBlock[index]; 
        }
        inline T& Front() { return m_pBlock[0]; }
        inline T& Back() { return m_pBlock[m_size - 1]; }
        inline T* Data() const { return m_pBlock; }


        // Copies the given value to every element in the dynamic array
        inline void Fill(T&& val)
        {
            for (size_t i = 0; i < m_size; ++i)
            {
                BlitzenCore::BlitMemCopy(&m_pBlock[i], &val, sizeof(T));
            }
        }

        void Resize(size_t newSize)
        {
            if (newSize > m_capacity)
            {
                RearrangeCapacity(newSize);
            }
            
            m_size = newSize;
        }

        void Reserve(size_t size)
        {
            if (size > m_capacity)
            {
                RearrangeCapacity(size);
            }
        }

        void PushBack(const T& newElement)
        {
            if (m_size >= m_capacity)
            {
                RearrangeCapacity(m_size + 1);
            }

            m_pBlock[m_size++] = newElement;
        }

        void AppendArray(DynamicArray<T>& array)
        {
            size_t additional = array.GetSize();
            if (m_size + additional > m_capacity)
            {
                RearrangeCapacity(m_size + additional);
            }
            BlitzenCore::BlitMemCopy(m_pBlock + m_size, array.Data(), additional * sizeof(T));
            m_size += additional;
        }

        
        void RemoveAtIndex(size_t index)
        {
            for (size_t i = index; i < m_size - 1; ++i)
            {
                m_pBlock[i] = m_pBlock[i + 1];
            }
            --m_size;
        }

        void Clear()
        {
            BlitzenCore::BlitZeroMemory(m_pBlock, m_size * sizeof(T));
            m_size = 0;
        }

        void DestroyManually()
        {
            BlitzenCore::BlitDestroyAlloc<T>(DArrayAlloc, m_pBlock, m_capacity);

            m_pBlock = nullptr;
            m_size = 0;
            m_capacity = 0;
        }

    private:

        void RearrangeCapacity(size_t newSize)
        {
            auto oldCapacity = m_capacity;
            m_capacity = newSize * ce_blitDynamiArrayCapacityMultiplier;
            T* pTemp = m_pBlock;

            m_pBlock = BlitzenCore::BlitAlloc<T>(DArrayAlloc, m_capacity);
            if (m_size != 0)
            {
                BlitzenCore::BlitMemCopy(m_pBlock, pTemp, m_size * sizeof(T));
                BlitzenCore::BlitFree<T>(DArrayAlloc, pTemp, oldCapacity);
            }
        }

    private:

        size_t m_size;
        
        size_t m_capacity;
        
        T* m_pBlock;
    };

    inline void FillArray(DynamicArray<uint32_t>& arr, uint32_t val)
    {
        BlitzenCore::BlitMemSet(arr.Data(), val, arr.GetSize());
    }
}