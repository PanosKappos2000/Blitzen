#include "Core/blitMemory.h"

namespace BlitCL
{
    template<typename T, BlitzenCore::AllocationType alloc = SpnAlloc>
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

        ~SmartPointer()
        {
            if (m_pData)
            {
                BlitzenCore::BlitDestroyAlloc(alloc, m_pData);
            }
        }

        // Allocates memory for an object
        template<typename... ARGS>
        void Make(ARGS&&... args)
        {
            m_pData = BlitzenCore::BlitConstructAlloc<T>(alloc, std::forward<ARGS>(args)...);
        }

        // Allocates memory for a derived class and assigns it to the base class pointer
        template<typename DERIVED, typename... ARGS>
        void MakeAs(ARGS&&... args)
        {
            m_pData = BlitzenCore::BlitConstructAlloc<DERIVED>(alloc, std::forward<ARGS>(args)...);
        }

        SmartPointer<T, alloc> operator = (const SmartPointer<T>& s) = delete;
        SmartPointer(const SmartPointer<T>& s) = delete;

        inline T* Data() { return m_pData; }
        inline T& Ref() { return *m_pData; }

        inline T* operator ->() { return m_pData; }
        inline T& operator *() { return *m_pData; }
        inline T** operator &() { return &m_pData; }

    private:
        T* m_pData;
    };
}