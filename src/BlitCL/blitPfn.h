#pragma once

namespace BlitCL
{
    // Lightweight Function pointer holder
    template<typename RET, typename... ARGS>
    class Pfn
    {
    public:

        using PfnType = RET(*)(ARGS...);

        Pfn() : m_func{ 0 } 
        {
        
        }

        Pfn(PfnType pfn) : m_func{ pfn } 
        {
        
        }

        void operator = (PfnType pfn) { m_func = pfn; }

        RET operator () (ARGS... args) { return m_func(std::forward<ARGS>(args)...); }

        bool IsFunctional() { return m_func != 0; }

    private:

        PfnType m_func;
    };
}