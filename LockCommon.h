#pragma once

namespace LockCommon
{

    template<typename T>
    inline bool CAS(T* actual, const T& expected, const T& desired)
    {
        T tmp = expected;
        return __atomic_compare_exchange(actual,&tmp,desired,false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);
    }

    template<typename T>
    inline bool CAS_ptr(T** actual, T** expected, T* desired)
    {
        return __atomic_compare_exchange_n(actual,expected,desired,false,__ATOMIC_SEQ_CST,__ATOMIC_SEQ_CST);
    }

    template<typename T>
    inline T FAS(T* target,const T* swap)
    {
        return __atomic_exchange_n(target,swap,__ATOMIC_SEQ_CST);
    }

    template<typename T>
    inline T* FAS_ptr(T** target,T* val)
    {
        return __atomic_exchange_n(target,val,__ATOMIC_SEQ_CST);
    }

};
