#pragma once
#include "Core/blitMemory.h"

namespace BlitCL
{
    constexpr uint8_t ce_blitDynamiArrayCapacityMultiplier = 2;
    constexpr BlitzenCore::AllocationType DArrayAlloc = BlitzenCore::AllocationType::DynamicArray;

    template<typename T>
    class DynamicArrayIterator
    {
    public:
        DynamicArrayIterator(T* ptr) :m_pElement{ ptr } {}

        inline bool operator !=(DynamicArrayIterator<T>& l) { return m_pElement != l.m_pElement; }

        inline DynamicArrayIterator<T>& operator ++() {
            m_pElement++;
            return *this;
        }

        inline DynamicArrayIterator<T>& operator ++(int) {
            DynamicArrayIterator<T> temp = *this;
            ++(*this);
            return temp;
        }

        inline DynamicArrayIterator<T>& operator --() {
            m_pElement--;
            return *this;
        }

        inline DynamicArrayIterator<T>& operator --(int) {
            DynamicArrayIterator<T> temp = *this;
            --(*this);
            return temp;
        }

        inline T& operator [](size_t idx) { return m_pElement[idx]; }

        inline T& operator *() { return *m_pElement; }

        inline T* operator ->() { return m_pElement; }

    private:
        T* m_pElement;
    };

    template<typename T>
    class DynamicArray
    {
    public:

        DynamicArray()
            :m_size{ 0 }, m_capacity{ 0 }, m_pBlock{ nullptr }
        {
        }

        DynamicArray(size_t initialSize)
            :m_size{ initialSize },
            m_capacity{ initialSize * ce_blitDynamiArrayCapacityMultiplier },
            m_pBlock{ BlitzenCore::BlitAlloc<T>(DArrayAlloc, m_capacity) }
        {
        }

        DynamicArray(size_t initialSize, const T& data)
            :m_size{ initialSize },
            m_capacity{ initialSize * ce_blitDynamiArrayCapacityMultiplier },
            m_pBlock{ BlitzenCore::BlitAlloc<T>(DArrayAlloc, m_capacity) }
        {
            for (size_t i = 0; i < initialSize; ++i)
                BlitzenCore::BlitMemCopy(&m_pBlock[i], &data, sizeof(T));
        }

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

        ~DynamicArray()
        {
            if (m_capacity > 0)
                BlitzenCore::BlitDestroyAlloc<T>(DArrayAlloc, m_pBlock, m_capacity);
        }



        using Iterator = DynamicArrayIterator<T>;
        inline Iterator begin() { return Iterator(m_pBlock); }
        inline Iterator end() { return Iterator(m_pBlock + m_size); }


        inline size_t GetSize() const { return m_size; }
        inline T& operator [] (size_t index) const{ return m_pBlock[index]; }
        inline T& Front() { m_pBlock[0]; }
        inline T& Back() { return m_pBlock[m_size - 1]; }
        inline T* Data() const { return m_pBlock; }


        // Copies the given value to every element in the dynamic array
        inline void Fill(T&& val)
        {
            for (size_t i = 0; i < m_size; ++i)
                BlitzenCore::BlitMemCopy(&m_pBlock[i], &val, sizeof(T));
        }

        void Resize(size_t newSize)
        {
            if (newSize < m_size)
            {
                m_size = newSize;
            }
            if (newSize > m_capacity)
            {
                RearrangeCapacity(newSize);
                m_size = newSize;
            }
        }

        void Reserve(size_t size)
        {
            RearrangeCapacity(size / ce_blitDynamiArrayCapacityMultiplier);
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

        // This one's terrible, should fix it when I am not bored
        void RemoveAtIndex(size_t index)
        {
            T* pTempBlock = m_pBlock;

            m_pBlock = BlitzenCore::BlitAlloc<T>(DArrayAlloc, m_capacity);
            BlitzenCore::BlitMemCopy(m_pBlock, pTempBlock, (index) * sizeof(T));
            BlitzenCore::BlitMemCopy(m_pBlock + index, pTempBlock + index + 1, (m_size - index - 1) * sizeof(T));
            BlitzenCore::BlitFree<T>(DArrayAlloc, pTempBlock, m_size);

            m_size--;
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

        size_t m_size;
        
        size_t m_capacity;
        
        T* m_pBlock;

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
    };
}