#pragma once
#include <cstdint>
#include <cstddef>

namespace BlitCL
{
    template<typename T>
    class DynamicArrayIterator
    {
    public:
        DynamicArrayIterator(T* ptr) :m_pElement{ ptr } {}

        inline bool operator !=(DynamicArrayIterator<T>& l) { return m_pElement != l.m_pElement; }

        inline DynamicArrayIterator<T>& operator ++() 
        {
            m_pElement++;
            return *this;
        }

        inline DynamicArrayIterator<T>& operator ++(int) {
            DynamicArrayIterator<T> temp = *this;
            ++(*this);
            return temp;
        }

        inline DynamicArrayIterator<T>& operator --() 
        {
            m_pElement--;
            return *this;
        }

        inline DynamicArrayIterator<T>& operator --(int) {
            DynamicArrayIterator<T> temp = *this;
            --(*this);
            return temp;
        }

        inline T& operator [](size_t idx) { return m_pElement[idx]; }

        inline T& operator *() const { return *m_pElement; }

        inline T* operator ->() const { return m_pElement; }

    private:
        T* m_pElement;
    };
}