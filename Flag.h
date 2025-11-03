#ifndef FLAG_H
#define FLAG_H

#include <cstdint>
#include <atomic>

struct Flag
{
    std::atomic<bool> innerFlag;
    //uint8_t padding[63];

    operator bool()
    {
        return innerFlag;
    }

    void operator=(bool val)
    {
        innerFlag = val;
    }
    
    bool get()
    { return innerFlag; }
};

struct PaddedAtomicInt
{
    std::atomic<uint32_t> innerVal;
    //uint8_t padding[60];

    void operator=(int val)
    {
        innerVal = val;
    }
    
    int get()
    { return innerVal; }
};

#endif
