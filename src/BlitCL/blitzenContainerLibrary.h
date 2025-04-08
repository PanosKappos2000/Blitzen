#pragma once
#include "Core/blitMemory.h"
#include "DynamicArray.h"
#include "blitArray.h"
#include "blitPfn.h"
#include "blitSmartPointer.h"
#include "blitHashMap.h"

namespace BlitCL
{
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