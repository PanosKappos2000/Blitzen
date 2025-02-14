#pragma once

#include "blitMemory.h"

#define BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER   2

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
                m_pBlock = BlitzenCore::BlitConstructAlloc<T, BlitzenCore::AllocationType::DynamicArray>(m_capacity);
            }
        }

        DynamicArray(size_t initialSize, T& data)
            :m_size{ initialSize }, m_capacity(initialSize* BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER)
        {
            if (m_size > 0)
            {
                m_pBlock = BlitzenCore::BlitAlloc<T>(BlitzenCore::AllocationType::DynamicArray, m_capacity);
                for (size_t i = 0; i < initialSize)
                    BlitzenCore::BlitMemCopy(&m_pBlock[i], &data, sizeof(T));
            }
        }

        DynamicArray(size_t initialSize, T&& data)
            :m_size{ initialSize }, m_capacity(initialSize* BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER)
        {
            if (m_size > 0)
            {
                m_pBlock = BlitzenCore::BlitAlloc<T>(BlitzenCore::AllocationType::DynamicArray, m_capacity);
                for (size_t i = 0; i < initialSize; ++i)
                    BlitzenCore::BlitMemCopy(&m_pBlock[i], &data, sizeof(T));
            }
        }

        DynamicArray(DynamicArray<T>& array)
            :m_size(array.GetSize()), m_capacity(array.GetSize() * BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER)
        {
            if (m_size > 0)
            {
                m_pBlock = BlitzenCore::BlitAlloc<T>(BlitzenCore::AllocationType::DynamicArray, m_capacity);
                BlitzenCore::BlitMemCopy(m_pBlock, array.Data(), array.GetSize() * sizeof(T));
            }
        }

        inline size_t GetSize() { return m_size; }

        inline T& operator [] (size_t index) { BLIT_ASSERT_DEBUG(index >= 0 && index < m_size) return m_pBlock[index]; }
        inline T& Front() { BLIT_ASSERT_DEBUG(m_size) m_pBlock[0]; }
        inline T& Back() { BLIT_ASSERT_DEBUG(m_size) return m_pBlock[m_size - 1]; }
        inline T* Data() {return m_pBlock; }

        void Fill(T value)
        {
            if(m_size > 0)
                BlitzenCore::BlitMemSet(m_pBlock, value, m_size * sizeof(T));
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
            BLIT_ASSERT_DEBUG(size)
            RearrangeCapacity(size / BLIT_DYNAMIC_ARRAY_CAPACITY_MULTIPLIER);
        }

        void PushBack(T& newElement)
        {
            // If the allocations have reached a point where the amount of elements is above the capacity, increase the capacity
            if(m_size + 1 > m_capacity)
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
                delete [] m_pBlock;
                BlitzenCore::LogFree(BlitzenCore::AllocationType::DynamicArray, m_capacity * sizeof(T));
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
            m_pBlock = BlitzenCore::BlitAlloc<T>(BlitzenCore::AllocationType::DynamicArray, m_capacity);
            if (m_size != 0)
            {
                BlitzenCore::BlitMemCopy(m_pBlock, pTemp, m_size * sizeof(T));
            }
            if(temp != 0)
            {
                delete [] pTemp;
                BlitzenCore::LogFree(BlitzenCore::AllocationType::DynamicArray, temp * sizeof(T));
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

        StaticArray(T& data)
        {
            static_assert(S > 0);

            for(size_t i = 0; i < S; ++i)
                BlitzenCore::BlitMemCopy(&m_pData[i], &data, sizeof(T));
        }

        StaticArray(T&& data)
        {
            static_assert(S > 0);

            for(size_t i = 0; i < S; ++i)
                BlitzenCore::BlitMemCopy(&m_pData[i], &data, sizeof(T));
        }

        inline T& operator [] (size_t idx) { BLIT_ASSERT(idx >= 0 && idx < S) return m_pData[idx]; }

        inline T* Data() { return m_pData; }

        inline size_t Size() { return S; }

        ~StaticArray()
        {
        
        }
    private:
        T m_pData[S];
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

        PointerTable(size_t fixedCapacity = 0)
            :m_capacity(fixedCapacity)
        {
            if(m_capacity > 0)
            {
                m_pBlock = reinterpret_cast<T**>(BlitzenCore::BlitAllocLinear(BlitzenCore::AllocationType::Hashmap, sizeof(T*) * m_capacity));
                BlitzenCore::BlitZeroMemory(m_pBlock, sizeof(T*) * m_capacity);
                m_hasMemory = 1;
            }
        }

        void SetCapacity(size_t capacity)
        {
            if(capacity > 0 && !m_hasMemory)
            {
                m_capacity = capacity;
                m_pBlock = reinterpret_cast<T**>(BlitzenCore::BlitAllocLinear(BlitzenCore::AllocationType::Hashmap, sizeof(T*) * m_capacity));
                BlitzenCore::BlitZeroMemory(m_pBlock, sizeof(T*) * m_capacity);
                m_hasMemory = 1;
            }
        }

        void Set(const char* name, T* pValue)
        {
            if(m_hasMemory)
            {
                size_t hash = Hash(name);
                m_pBlock[hash] = pValue ? pValue : nullptr;
            }
        }

        T* Get(const char* name, T* pDefault)
        {
            if(m_hasMemory)
            {
                size_t  hash = Hash(name);
                return m_pBlock[hash] ? m_pBlock[hash] : pDefault;
            }
            else
            {
                return pDefault;
            }
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
        uint8_t m_hasMemory = 0;

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

    

    template<typename T, // The type of pointer stored
    BlitzenCore::AllocationType A = BlitzenCore::AllocationType::SmartPointer, // The allocation type that the allocator should keep track of
    typename Ret = void, // The type that the custom destructor returns>
    typename... P>
    class SmartPointer
    {
    public:

        typedef Ret(*DstrPfn)(T*);

        SmartPointer(T* pDataToCopy = nullptr, DstrPfn customDestructor = nullptr)
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

            m_customDestructor = customDestructor;
        }

        SmartPointer(const T& data)
        {
            m_pData = BlitzenCore::BlitConstructAlloc<T, A>(data);
        }

        SmartPointer(T&& data)
        {
            m_pData = BlitzenCore::BlitConstructAlloc<T, A>(std::move(data));
        }

        SmartPointer(DstrPfn customDestructor, P&... params)
        {
            m_pData = BlitzenCore::BlitCOnstructAlloc<T>(A, params...);

            m_customDestructor = customDestructor;
        }

        inline T* Data() { return m_pData; }

        ~SmartPointer()
        {
            // Call the additional destructor function if it was given on construction
            if(m_customDestructor)
            {
                if(m_pData)
                    m_customDestructor(m_pData);

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
        DstrPfn m_customDestructor;
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