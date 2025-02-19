#pragma once

#include "blitMemory.h"

#define BLIT_ARRAY_SIZE(array)   sizeof(array) / sizeof(array[0])

namespace BlitCL
{
    constexpr uint8_t ce_blitDynamiArrayCapacityMultiplier = 2;

    inline uint32_t Clamp(uint32_t initial, uint32_t upper, uint32_t lower) { 
        return (initial >= upper) ? upper /* return upper if the value is above it */ 
        : (initial <= lower) ? lower /* Else return lower if the value is below it*/ 
        : initial; /* Else just return the initial value */
    }


    template<typename T>
    class DynamicArrayIterator
    {
    public:
        DynamicArrayIterator(T* ptr) :m_pElement{ptr}{}

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
            :m_size{initialSize}, m_capacity(initialSize * ce_blitDynamiArrayCapacityMultiplier)
        {
            if (m_size > 0)
            {
                m_pBlock = BlitzenCore::BlitConstructAlloc<T, BlitzenCore::AllocationType::DynamicArray>(m_capacity);
            }
        }

        DynamicArray(size_t initialSize, T& data)
            :m_size{ initialSize }, m_capacity(initialSize* ce_blitDynamiArrayCapacityMultiplier)
        {
            if (m_size > 0)
            {
                m_pBlock = BlitzenCore::BlitAlloc<T>(BlitzenCore::AllocationType::DynamicArray, m_capacity);
                for (size_t i = 0; i < initialSize; ++i)
                    BlitzenCore::BlitMemCopy(&m_pBlock[i], &data, sizeof(T));
            }
        }

        DynamicArray(size_t initialSize, T&& data)
            :m_size{ initialSize }, m_capacity(initialSize* ce_blitDynamiArrayCapacityMultiplier)
        {
            if (m_size > 0)
            {
                m_pBlock = BlitzenCore::BlitAlloc<T>(BlitzenCore::AllocationType::DynamicArray, m_capacity);
                for (size_t i = 0; i < initialSize; ++i)
                    BlitzenCore::BlitMemCopy(&m_pBlock[i], &data, sizeof(T));
            }
        }

        DynamicArray(DynamicArray<T>& array)
            :m_size(array.GetSize()), m_capacity(array.GetSize() * ce_blitDynamiArrayCapacityMultiplier)
        {
            if (m_size > 0)
            {
                m_pBlock = BlitzenCore::BlitAlloc<T>(BlitzenCore::AllocationType::DynamicArray, m_capacity);
                BlitzenCore::BlitMemCopy(m_pBlock, array.Data(), array.GetSize() * sizeof(T));
            }
        }

        using Iterator = DynamicArrayIterator<T>;
        inline Iterator begin() { return Iterator(m_pBlock); }
        inline Iterator end() { return Iterator(m_pBlock + m_size); }

        inline size_t GetSize() { return m_size; }

        inline T& operator [] (size_t index) { BLIT_ASSERT(index >= 0 && index < m_size) return m_pBlock[index]; }
        inline T& Front() { BLIT_ASSERT(m_size) m_pBlock[0]; }
        inline T& Back() { BLIT_ASSERT(m_size) return m_pBlock[m_size - 1]; }
        inline T* Data() {return m_pBlock; }

        void Fill(const T& val)
        {
            if(m_size > 0)
                for(size_t i = 0; i < m_size; ++i)
                    Memcpy(&m_pBlock[i], &val, sizeof(T));
        }

        void Resize(size_t newSize)
        {
            // A different function will be used for downsizing
            if(newSize < m_size)
            {
                return;
            }
            // If the allocations have reached a point where the amount of elements is above the capacity, increase the capacity
            if(newSize > m_capacity)
            {
                RearrangeCapacity(newSize);
                // TODO: Maybe I would want to zero out the memory after m_size and up to capacity
            }

            m_size = newSize;
        }

        void Downsize(size_t newSize)
        {
            if(newSize > m_size)
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
            if(m_size >= m_capacity)
            {
                RearrangeCapacity(m_size + 1);
            }

            m_pBlock[m_size++] = newElement;
        }

        void AddBlockAtBack(T* pNewBlock, size_t blockSize)
        {
            if(m_size + blockSize > m_capacity)
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
            Memcpy(m_pBlock + additional, array.Data(), additional * sizeof(T));
            // Adds this array's size to m_size
            m_size += additional;
        }
        

        void RemoveAtIndex(size_t index)
        {
            if(index < m_size && index >= 0)
            {
                T* pTempBlock = m_pBlock;

                m_pBlock = BlitzenCore::BlitAlloc<T>(BlitzenCore::AllocationType::DynamicArray, m_capacity);

                BlitzenCore::BlitMemCopy(m_pBlock, pTempBlock, (index) * sizeof(T));

                BlitzenCore::BlitMemCopy(m_pBlock + index, pTempBlock + index + 1, (m_size - index) * sizeof(T));

                BlitzenCore::BlitFree<T>(BlitzenCore::AllocationType::DynamicArray, pTempBlock, m_size);

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
                BlitzenCore::BlitDestroyAlloc<T>(BlitzenCore::AllocationType::DynamicArray, m_pBlock, m_capacity);
            }
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
            size_t temp = m_capacity;
            m_capacity = newSize * ce_blitDynamiArrayCapacityMultiplier;
            T* pTemp = m_pBlock;
            m_pBlock = BlitzenCore::BlitAlloc<T>(BlitzenCore::AllocationType::DynamicArray, m_capacity);
            if (m_size != 0)
            {
                BlitzenCore::BlitMemCopy(m_pBlock, pTemp, m_size * sizeof(T));
            }
            if(temp != 0)
            {
                BlitzenCore::BlitDestroyAlloc<T>(BlitzenCore::AllocationType::DynamicArray, pTemp, temp);
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

            for(size_t i = 0; i < S; ++i)
                BlitzenCore::BlitMemCopy(&m_array[i], &data, sizeof(T));
        }

        StaticArray(T&& data)
        {
            static_assert(S > 0);

            for(size_t i = 0; i < S; ++i)
                BlitzenCore::BlitMemCopy(&m_array[i], &data, sizeof(T));
        }

        using Iterator = DynamicArrayIterator<T>;
        Iterator begin() { return Iterator(m_array); }
        Iterator end() { return Iterator(m_array * S); }

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
            :m_capacity(initialCapacity)
        {
            if(m_capacity > 0)
            {
                m_pBlock = BlitzenCore::BlitConstructAlloc<T, BlitzenCore::AllocationType::Hashmap>(m_capacity);
            }
        }

        void Insert(const char* name, const T& value)
        {
            if(m_capacity <= m_elementCount)
            {
                IncreaseCapacity(m_capacity + 1);
            }

            size_t hash = Hash(name);
            m_pBlock[hash] = value;
            ++m_elementCount;
        }

        inline T& operator [](const char* name){
            size_t  hash = Hash(name);
            return m_pBlock[hash];
        }

        ~HashMap()
        {
            if(m_capacity > 0)
            {
                delete [] m_pBlock;
                BlitzenCore::LogFree(BlitzenCore::AllocationType::DynamicArray, m_capacity * sizeof(T));
            }
        }

    private:

        size_t m_capacity;
        size_t m_elementCount;

        T* m_pBlock;

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
                delete [] pTemp;
                BlitzenCore::LogFree(BlitzenCore::AllocationType::Hashmap, temp * sizeof(T));
            }
        }
    };

    

    template<typename T, BlitzenCore::AllocationType A = BlitzenCore::AllocationType::SmartPointer>
    class SmartPointer
    {
    public:

        using Dstr =  void(*)(T*);

        SmartPointer(T* pDataToCopy = nullptr)
        {
            // Allocated on the heap
            m_pData = BlitzenCore::BlitConstructAlloc<T>(A);

            if(pDataToCopy)
            {
                // Copy the data over to the member variable
                BlitzenCore::BlitMemCopy(m_pData, pDataToCopy, sizeof(T));
                // Redirect the pointer, in case the user wants to use it again
                pDataToCopy = m_pData;
            }
        }

        SmartPointer(const T& data)
        {
            m_pData = BlitzenCore::BlitConstructAlloc<T, A>(data);
        }

        SmartPointer(T&& data)
        {
            m_pData = BlitzenCore::BlitConstructAlloc<T, A>(std::move(data));
        }

        // This should not be a member function, but I am not using it now, so I will change it later
        template<typename... P>
        T* MakeSmartPointer(P... params)
        {
            m_pData = BlitzenCore::BlitConstructAlloc<T>(A, params...);
        }

        inline void SetCustomDestructor(Dstr func) { m_pfnDstr = func; }

        inline T* Data() { return m_pData; }

        inline T* operator ->() {return m_pData; }

        ~SmartPointer()
        {
            // Call the additional destructor function if it was given on construction
            if(m_pfnDstr)
            {
                if(m_pData)
                    m_pfnDstr(m_pData);

                // The smart pointer trusts that the custom destructor did its job and free the block of memory
                BlitzenCore::LogFree(A, sizeof(T));
            }

            // Does the job using delete, if the user did not provide a custom destructor
            else
                BlitzenCore::BlitDestroyAlloc(A, m_pData);
        }
    private:
        T* m_pData;

        // Additional destructor function currently fixed type (void(*)(T*))
        Dstr m_pfnDstr = 0;
    };

    // Allocates a set amount of size on the heap, until the instance goes out of scope (Constructors not called)
    template<typename T, BlitzenCore::AllocationType A>
    class StoragePointer
    {
    public:

        StoragePointer(size_t size = 0)
        {
            if(size > 0)
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
            if(m_pData && m_size > 0)
            {
                BlitzenCore::BlitFree<T>(A, m_pData, m_size);
            }
        }

    private:
        T* m_pData = nullptr;

        size_t m_size;
    };
}