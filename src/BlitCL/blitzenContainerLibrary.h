#pragma once

#include "Core/blitMemory.h"
#include "DynamicArray.h"

#define BLIT_ARRAY_SIZE(array)   sizeof(array) / sizeof(array[0])

namespace BlitCL
{

	constexpr uint8_t ce_blitStringCapacityMultiplier = 2;

    constexpr BlitzenCore::AllocationType StrAlloc = BlitzenCore::AllocationType::String;

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

        inline T& operator [] (size_t idx) { return m_array[idx]; }

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
                BlitzenCore::BlitFree<T>(BlitzenCore::AllocationType::Hashmap, pTemp, temp);
            }
        }
    };



    template<typename T, BlitzenCore::AllocationType alloc = BlitzenCore::AllocationType::SmartPointer>
    class SmartPointer
    {
    public:

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
            m_pData = BlitzenCore::BlitConstructAlloc<T>(alloc, params...);
        }

        // Allocates memory for a derived class and assigns it to the base class pointer
        template<typename I, typename... P>
        void MakeAs(const P&... params)
        {
            m_pData = BlitzenCore::BlitConstructAlloc<I>(alloc, params...);
        }

        SmartPointer<T, alloc> operator = (SmartPointer<T>& s) = delete;
        SmartPointer<T, alloc> operator = (SmartPointer<T> s) = delete;
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
            if(m_size > 0)
            {
                m_pData = BlitzenCore::BlitAlloc<T>(A, size);
                m_size = size;
            }
        }

        void TransferOwnership(T* pData, size_t size)
        {
            if (m_size > 0)
            {
                m_pData = pData;
                m_size = size;
            }
        }

        inline T* Data() { return m_pData; }

        inline uint8_t IsEmpty() { return m_size == 0; }

        StoragePointer<T, A> operator = (StoragePointer<T, A>& s) = delete;
        StoragePointer<T, A> operator = (StoragePointer<T, A> s) = delete;
        StoragePointer(const StoragePointer<T, A>& s) = delete;
        StoragePointer(StoragePointer<T, A>& s) = delete;

        ~StoragePointer()
        {
            if (m_size > 0)
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

    class String
    {
    public:
        inline String() :
            m_data{ nullptr }, 
            m_capacity{ 0 }, 
            m_size{ 0 }
        {}

        inline String(char* data) 
        {
            m_size = strlen(data);
            m_capacity = m_size * ce_blitStringCapacityMultiplier + 1;
            m_data = BlitzenCore::BlitAlloc<char>(StrAlloc, m_capacity);
            strcpy(m_data, data);
        }

        inline ~String()
		{
			if (m_capacity)
			{
				BlitzenCore::BlitFree<char>(StrAlloc, m_data, m_capacity);
			}
		}

        inline char operator [] (size_t idx) const { return m_data[idx]; }
        inline const char* GetClassic() const { return m_data; }
        inline size_t GetSize() const { return m_size; }
		inline size_t GetCapacity() const { return m_capacity; }

        inline void Append(char* str)
        {
            size_t newSize = strlen(str) + m_size;
			if (m_capacity <= newSize)
			{
				IncreaseCapacity(newSize);
			}
            strcpy(m_data + m_size, str);
            m_size = newSize;
        }

    private:

        void IncreaseCapacity(size_t newSize)
        {
            auto previousCapacity = m_capacity;
            BlitCL::StoragePointer<char, StrAlloc> previousData{};
            previousData.TransferOwnership(m_data, m_size);
            m_capacity = newSize * ce_blitStringCapacityMultiplier + 1;

            m_data = BlitzenCore::BlitAlloc<char>(StrAlloc, m_capacity);
            if (m_size != 0)
            {
                strcpy(m_data, previousData.Data());
            }
        }

    private:
        char* m_data;

        size_t m_capacity;

        size_t m_size;
    };
}