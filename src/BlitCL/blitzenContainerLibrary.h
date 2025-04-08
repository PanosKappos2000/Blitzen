#pragma once

#include "Core/blitMemory.h"
#include "DynamicArray.h"
#include "blitString.h"
#include "blitArray.h"
#include "blitPfn.h"

#define BLIT_ARRAY_SIZE(array)   sizeof(array) / sizeof(array[0])

namespace BlitCL
{
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

    // The Unkown Pointer is a weak version of the smart pointer, that uses void* instead of templates
    // Useful for very specific scenarios, applications should choose smart pointer when possible
    class UnknownPointer
    {
    public:

        inline UnknownPointer(void* pData, size_t size)
        {
            m_size = size;
            m_pData = BlitzenCore::BlitAlloc<uint8_t>(BlitzenCore::AllocationType::SmartPointer, size);
        }

        inline UnknownPointer() : m_pData{ nullptr }, m_size{ 0 }
        {}

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
}