#pragma once

#include <sched.h>

static uint32_t sT = std::thread::hardware_concurrency();

template<class CohFunc>
static void PrintCohortFunction(uint32_t t = 0)
{
    int T;
    if(t == 0)
    {
        T = std::thread::hardware_concurrency();
    }
    else
    {
        T = t;
    }

    for(int i = 0; i < T; i++)
    {
        printf("%d->%d\n",i,CohFunc::getCidVal(i,T));
    }
}

template <uint32_t N>
class NumaN_NHT
{
public:
    static uint32_t cf()
    {
        int cpuId = sched_getcpu();

        static thread_local uint32_t cid = getCidVal(cpuId);
        return cid;
    }

    static uint32_t getCidVal(int cpuId,int t = 0)
    {
        if(t == 0)
        {
            t = sT;
        }
        return cpuId / (t/N); 
    }
};

template <uint32_t N>
class NumaN_HT
{
public:
    static uint32_t cf()
    {
        int cpuId = sched_getcpu();

        static thread_local uint32_t cid = getCidVal(cpuId);
        return cid;
    }

    static uint32_t getCidVal(int cpuId,int t = 0)
    {
        if(t == 0)
        {
            t = sT;
        }
        uint32_t cores = t / 2;
        uint32_t numaSize = cores / N;
        return (cpuId % cores) / numaSize;
    }
};
