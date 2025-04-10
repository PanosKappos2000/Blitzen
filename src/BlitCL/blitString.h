#pragma once
#include "Core/blitMemory.h"
#include <string.h>
#include <cstdint>

namespace BlitCL
{
    constexpr uint8_t ce_blitStringCapacityMultiplier = 2;

    constexpr BlitzenCore::AllocationType StrAlloc = BlitzenCore::AllocationType::String;

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
            if(m_size == 0)
            {
                m_pData = BlitzenCore::BlitAlloc<T>(A, size);
                m_size = size;
            }
        }

        void TransferOwnership(T* pData, size_t size)
        {
            if (m_size == 0)
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

    class String
    {
    public:
        inline String() :
            m_data{ nullptr }, 
            m_capacity{ 0 }, 
            m_size{ 0 }
        {}

        inline String(const char* data) 
        {
            m_size = strlen(data);
            m_capacity = m_size * ce_blitStringCapacityMultiplier + 1;
            m_data = BlitzenCore::BlitAlloc<char>(StrAlloc, m_capacity);
            strcpy_s(m_data, m_size + 1, data);
        }

        inline String(size_t size)
        {
            m_size = size;
            m_capacity = m_size * ce_blitStringCapacityMultiplier + 1;
            m_data = BlitzenCore::BlitAlloc<char>(StrAlloc, m_capacity);
        }

        inline String(const String& str) = delete;
        inline String operator = (const String& str) = delete;

        inline ~String()
		{
			if (m_capacity)
			{
				BlitzenCore::BlitFree<char>(StrAlloc, m_data, m_capacity);
			}
		}

        inline char operator [] (size_t idx) const { return m_data[idx]; }
        inline const char* GetClassic() const { return m_data; }
        inline char* Data() { return m_data; }
        inline size_t GetSize() const { return m_size; }
		inline size_t GetCapacity() const { return m_capacity; }

		inline void Resize(size_t size)
		{
			if (size >= m_capacity)
			{
				IncreaseCapacity(size);
			}
			m_size = size;
		}

        inline void Append(char* str)
        {
			size_t strSize = strlen(str);
            size_t newSize = strSize + m_size;
			if (m_capacity <= newSize)
			{
				IncreaseCapacity(newSize);
			}
            strcpy_s(m_data + m_size, strSize + 1, str);
            m_size = newSize;
        }

        inline int64_t FindLastOf(char c)
        {
            for (int64_t i = m_size; i >= 0; --i)
            {
                if (m_data[i] == c)
                    return i;
            }

            return -1;
        }

        inline void CopyString(const char* str)
        {
            size_t size = strlen(str);
            if(size >= m_capacity)
            {
                IncreaseCapacity(size);
            }
            strcpy_s(m_data, size + 1, str);
        }

        inline String Substring(size_t start, size_t size)
        {
            BlitCL::StoragePointer<char, StrAlloc> newStringData{};
            newStringData.AllocateStorage(size + 1);
            strncpy_s(newStringData.Data(), size + 1, m_data + start, size);

            return String{ newStringData.Data() };
        }

        inline void ReplaceSubstring(size_t start, char* str)
        {
			size_t strSize = strlen(str);
			BlitzenCore::BlitMemCopy<char>(m_data + start, str, strSize);
        }

    private:

        void IncreaseCapacity(size_t newSize)
        {
            auto previousCapacity = m_capacity;
            const char* previousData = m_data;
            m_capacity = newSize * ce_blitStringCapacityMultiplier + 1;

            m_data = BlitzenCore::BlitAlloc<char>(StrAlloc, m_capacity);
            if (m_size != 0)
            {
                strcpy_s(m_data, m_size + 1, previousData);
            }
        }

    private:
        char* m_data;

        size_t m_capacity;

        size_t m_size;
    };
}
