#pragma once

namespace BlitCL
{
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