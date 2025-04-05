#pragma once

#include "Core/blitMemory.h"

#define BLIT_ARRAY_SIZE(array)   sizeof(array) / sizeof(array[0])

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

        DynamicArray(size_t initialSize = 0)
            :m_size{ initialSize }, m_capacity(initialSize* ce_blitDynamiArrayCapacityMultiplier)
        {
            if (m_size > 0)
            {
                m_pBlock = BlitzenCore::BlitConstructAlloc<T, DArrayAlloc>(m_capacity);
            }
        }

        DynamicArray(size_t initialSize, T& data)
            :m_size{ initialSize }, m_capacity(initialSize* ce_blitDynamiArrayCapacityMultiplier)
        {
            if (m_size > 0)
            {
                m_pBlock = BlitzenCore::BlitAlloc<T>(DArrayAlloc, m_capacity);
                for (size_t i = 0; i < initialSize; ++i)
                    BlitzenCore::BlitMemCopy(&m_pBlock[i], &data, sizeof(T));
            }
        }

        DynamicArray(size_t initialSize, T&& data)
            :m_size{ initialSize }, m_capacity(initialSize* ce_blitDynamiArrayCapacityMultiplier)
        {
            if (m_size > 0)
            {
                m_pBlock = BlitzenCore::BlitAlloc<T>(DArrayAlloc, m_capacity);
                for (size_t i = 0; i < initialSize; ++i)
                    BlitzenCore::BlitMemCopy(&m_pBlock[i], &data, sizeof(T));
            }
        }

        DynamicArray(DynamicArray<T>& array)
            :m_size(array.GetSize()), m_capacity(array.GetSize()* ce_blitDynamiArrayCapacityMultiplier)
        {
            if (m_size > 0)
            {
                m_pBlock = BlitzenCore::BlitAlloc<T>(DArrayAlloc, m_capacity);
                BlitzenCore::BlitMemCopy(m_pBlock, array.Data(), array.GetSize() * sizeof(T));
            }
        }

        using Iterator = DynamicArrayIterator<T>;
        inline Iterator begin() { return Iterator(m_pBlock); }
        inline Iterator end() { return Iterator(m_pBlock + m_size); }

        inline size_t GetSize() const { return m_size; }

        inline T& operator [] (size_t index) const
        {
            BLIT_ASSERT(index >= 0 && index < m_size)
                return m_pBlock[index];
        }

        inline T& Front() { BLIT_ASSERT(m_size) m_pBlock[0]; }
        inline T& Back() { BLIT_ASSERT(m_size) return m_pBlock[m_size - 1]; }
        inline T* Data() const { return m_pBlock; }

        // Copies the given value to every element in the dynamic array
        void Fill(T&& val)
        {
            if (m_size > 0)
                for (size_t i = 0; i < m_size; ++i)
                    BlitzenCore::BlitMemCopy(&m_pBlock[i], &val, sizeof(T));
        }

        // TODO: I should have this be for both resize and downsize, I do not know what I was thinking
        void Resize(size_t newSize)
        {
            if (newSize < m_size)
            {
                return;
            }
            if (newSize > m_capacity)
            {
                RearrangeCapacity(newSize);
            }
            m_size = newSize;
        }

        void Downsize(size_t newSize)
        {
            if (newSize > m_size)
            {
                return;
            }
            m_size = newSize;
        }

        void Reserve(size_t size)
        {
            BLIT_ASSERT(size > m_capacity)
                RearrangeCapacity(size / ce_blitDynamiArrayCapacityMultiplier);
        }

        void PushBack(const T& newElement)
        {
            // If the allocations have reached a point where the amount of elements is above the capacity, increase the capacity
            if (m_size >= m_capacity)
            {
                RearrangeCapacity(m_size + 1);
            }

            m_pBlock[m_size++] = newElement;
        }

        void AddBlockAtBack(T* pNewBlock, size_t blockSize)
        {
            if (m_size + blockSize > m_capacity)
            {
                RearrangeCapacity(m_size + blockSize);
            }
            BlitzenCore::BlitMemCopy(m_pBlock + m_size, pNewBlock, blockSize * sizeof(T));
            m_size += blockSize;
        }

        void AppendArray(DynamicArray<T>& array)
        {
            size_t additional = array.GetSize();

            // If there is not enough space allocates more first
            if (m_size + additional > m_capacity)
            {
                RearrangeCapacity(m_size + additional);
            }

            // Copies the new array's data to this array
            BlitzenCore::BlitMemCopy(m_pBlock + m_size, array.Data(), additional * sizeof(T));
            // Adds this array's size to m_size
            m_size += additional;
        }

        // This one's terrible, should fix it when I am not bored
        void RemoveAtIndex(size_t index)
        {
            if (index < m_size && index >= 0)
            {
                T* pTempBlock = m_pBlock;

                m_pBlock = BlitzenCore::BlitConstructAlloc<T, DArrayAlloc>(m_capacity);

                BlitzenCore::BlitMemCopy(m_pBlock, pTempBlock, (index) * sizeof(T));

                BlitzenCore::BlitMemCopy(m_pBlock + index, pTempBlock + index + 1, (m_size - index) * sizeof(T));

                BlitzenCore::BlitFree<T>(DArrayAlloc, pTempBlock, m_size);

                m_size--;
            }
        }

        void Clear()
        {
            if (m_size)
            {
                BlitzenCore::BlitZeroMemory(m_pBlock, m_size * sizeof(T));
                m_size = 0;
            }
        }

        void DestroyManually()
        {
            if (m_capacity > 0)
            {
                BlitzenCore::BlitDestroyAlloc<T>(DArrayAlloc, m_pBlock, m_capacity);
                m_pBlock = nullptr;
                m_size = 0;
                m_capacity = 0;
            }
        }

        ~DynamicArray()
        {
            if (m_capacity > 0)
                BlitzenCore::BlitDestroyAlloc<T>(DArrayAlloc, m_pBlock, m_capacity);
        }

    private:

        // The array size that is currently being worked with
        size_t m_size;
        // The actual size of the allocation
        size_t m_capacity;
        // Pointer to the start of the array
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




    template<typename T, size_t S>
    class StaticArray
    {
    public:
        StaticArray()
        {
            static_assert(S > 0);
        }

        StaticArray(const T& data)
        {
            static_assert(S > 0);

            for (size_t i = 0; i < S; ++i)
                BlitzenCore::BlitMemCopy(&m_array[i], &data, sizeof(T));
        }

        StaticArray(T&& data)
        {
            static_assert(S > 0);

            for (size_t i = 0; i < S; ++i)
                BlitzenCore::BlitMemCopy(&m_array[i], &data, sizeof(T));
        }

        using Iterator = DynamicArrayIterator<T>;
        Iterator begin() { return Iterator(m_array); }
        Iterator end() { return Iterator(m_array + S); }

        inline T& operator [] (size_t idx) { BLIT_ASSERT(idx >= 0 && idx < S) return m_array[idx]; }

        inline T* Data() { return m_array; }

        inline size_t Size() { return S; }

        ~StaticArray()
        {

        }
    private:
        T m_array[S];
    };


    inline void FillArray(DynamicArray<uint32_t>& arr, uint32_t val)
    {
        if (arr.GetSize() > 0)
            BlitzenCore::BlitMemSet(arr.Data(), val, arr.GetSize());
    }



    /*------------------------------------------------------------------------------------------
        This structure is a very simple hash table that only takes pointers.
        It can never own any memory, so the objects it point to should be managed outside.
        It also only accepts names for keys. Better hash tables will be created at a later time
    ---------------------------------------------------------------------------------------------*/
    template<typename T>
    class HashMap
    {
    public:

        HashMap(size_t initialCapacity = 10)
            :m_capacity{ initialCapacity }, m_elementCount{ 0 }
        {
            if (m_capacity > 0)
            {
                m_pBlock = BlitzenCore::BlitConstructAlloc<T, BlitzenCore::AllocationType::Hashmap>(m_capacity);
            }
        }

        void Insert(const char* name, const T& value)
        {
            if (m_capacity <= m_elementCount + 1)
            {
                IncreaseCapacity(m_capacity + 1);
            }

            m_pBlock[HashFunction(name)] = value;
            ++m_elementCount;
        }

        inline const T& operator [](const char* name) { return m_pBlock[HashFunction(name)]; }
        inline T& GetVar(const char* name) { return m_pBlock[HashFunction(name)]; }

        ~HashMap()
        {
            if (m_capacity > 0)
            {
                BlitzenCore::BlitDestroyAlloc<T>(BlitzenCore::AllocationType::Hashmap, m_pBlock, m_capacity);
            }
        }

    private:

        size_t m_capacity;
        size_t m_elementCount;

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
            m_pBlock = BlitzenCore::BlitAlloc<T>(BlitzenCore::AllocationType::Hashmap, m_capacity);

            if (temp != 0)
            {
                BlitzenCore::BlitMemCopy(m_pBlock, pTemp, temp * sizeof(T));
                delete[] pTemp;
                BlitzenCore::LogFree(BlitzenCore::AllocationType::Hashmap, temp * sizeof(T));
            }
        }
    };



    template<typename T, BlitzenCore::AllocationType alloc = BlitzenCore::AllocationType::SmartPointer>
    class SmartPointer
    {
    public:

        // This might be something that I should remove, since Smart pointers handle allocations themselves
        SmartPointer(T* pDataToCopy)
        {
            BLIT_ASSERT(!pDataToCopy)
                m_pData = BlitzenCore::BlitConstructAlloc<T, alloc>(pDataToCopy);
        }

        // Default constructor
        SmartPointer() : m_pData{ nullptr } {}

        // Copy constructor
        SmartPointer(const T& data)
        {
            m_pData = BlitzenCore::BlitConstructAlloc<T, alloc>(data);
        }

        // Move contructor
        SmartPointer(T&& data)
        {
            m_pData = BlitzenCore::BlitConstructAlloc<T, alloc>(std::move(data));
        }

        // Allocates memory for an object
        template<typename... P>
        void Make(const P&... params)
        {
            BLIT_ASSERT(!m_pData);
            m_pData = BlitzenCore::BlitConstructAlloc<T>(alloc, params...);
        }

        // Allocates memory for a derived class and assigns it to the base class pointer
        template<typename I, typename... P>
        void MakeAs(const P&... params)
        {
            BLIT_ASSERT(!m_pData);
            m_pData = BlitzenCore::BlitConstructAlloc<I>(alloc, params...);
        }

        SmartPointer<T> operator = (SmartPointer<T>& s) = delete;
        SmartPointer<T> operator = (SmartPointer<T> s) = delete;
        SmartPointer(const SmartPointer<T>& s) = delete;
        SmartPointer(SmartPointer<T>& s) = delete;

        inline T* Data() { return m_pData; }
        inline T& Ref() { return m_pData[0]; }
        inline T* operator ->() { return m_pData; }
        inline T& operator *() { return *m_pData; }
        inline T** operator &() { return &m_pData; }

        ~SmartPointer()
        {
            if (m_pData)
            {
                BlitzenCore::BlitDestroyAlloc(alloc, m_pData);
            }
        }

    private:
        T* m_pData;
    };

    // Allocates a set amount of size on the heap, until the instance goes out of scope (Constructors not called)
    template<typename T, BlitzenCore::AllocationType A>
    class StoragePointer
    {
    public:

        StoragePointer(size_t size = 0)
        {
            if (size > 0)
            {
                m_pData = BlitzenCore::BlitAlloc<T>(A, size);
            }
            m_size = size;
        }

        void AllocateStorage(size_t size)
        {
            BLIT_ASSERT_MESSAGE(m_size == 0, "The storage has already allocated storage")

                m_pData = BlitzenCore::BlitAlloc<T>(A, size);
            m_size = size;
        }

        inline T* Data() { return m_pData; }

        inline uint8_t IsEmpty() { return m_size == 0; }

        ~StoragePointer()
        {
            if (m_pData && m_size > 0)
            {
                BlitzenCore::BlitFree<T>(A, m_pData, m_size);
            }
        }

    private:
        T* m_pData = nullptr;

        size_t m_size;
    };

    class UnknownPointer
    {
    public:

        inline UnknownPointer(void* pData, size_t size)
        {
            BLIT_ASSERT(pData && size)
                m_size = size;
            m_pData = BlitzenCore::BlitAlloc<uint8_t>(BlitzenCore::AllocationType::SmartPointer, size);
        }

        inline UnknownPointer() : m_pData{ nullptr }, m_size{ 0 }
        {
        }

        inline void Make(void* pData, size_t size)
        {
            m_size = size;
            m_pData = BlitzenCore::BlitAlloc<uint8_t>(BlitzenCore::AllocationType::SmartPointer, size);
            BlitzenCore::BlitMemCopy(m_pData, pData, size);
        }

        template<typename T>
        inline T* Get() { return reinterpret_cast<T*>(m_pData); }

        inline ~UnknownPointer()
        {
            if (m_pData)
                BlitzenCore::BlitFree<uint8_t>(BlitzenCore::AllocationType::SmartPointer, m_pData, m_size);
        }

    private:
        void* m_pData;
        size_t m_size;
    };

    // Lightweight Function pointer holder
    template<typename RET, typename... Args>
    class Pfn
    {
    public:

        using PfnType = RET(*)(Args...);

        Pfn() : m_func{ 0 } {}

        Pfn(PfnType pfn) : m_func{ pfn } {}

        inline Pfn operator = (PfnType pfn) { return Pfn{ pfn }; }

        inline RET operator () (Args... args) { return m_func(args...); }

        inline bool IsFunctional() { return m_func != 0; }

    private:

        PfnType m_func;
    };
}