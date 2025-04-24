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