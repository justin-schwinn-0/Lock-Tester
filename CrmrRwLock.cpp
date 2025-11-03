#include "CrmrRwLock.h"

#include <thread>


CrmrRwLock::CrmrRwLock():
    mD(0),
    mExitPermit({false}),
    mPermit{{true},{false}},
    mGate{{true},{false}},
    mExitCount(Counter()),
    mCount{Counter(),Counter()},
    mLock(McsLock(0))
{
}
CrmrRwLock::~CrmrRwLock()
{
}

void CrmrRwLock::writeLock()
{
    mLock.aquire(0);
    int prevD = mD.load();
    int currD = 1 - prevD;
    mD.store(currD);

    mPermit[prevD].store(false);


    if(mCount[prevD].FAI(1,0) != std::pair<uint32_t,uint32_t>(0,0))
    {
        while(!mPermit[prevD].load())
        {
            std::this_thread::yield();
        }
    }

    mCount[prevD].FAD(1,0);

    mGate[prevD].store(false);
    mExitPermit.store(false);


    if(mExitCount.FAI(1,0) != std::pair<uint32_t,uint32_t>(0,0))
    {
        while(!mExitPermit.load())
        {
            std::this_thread::yield();
        }
    }

    mExitCount.FAD(1,0);

}

void CrmrRwLock::writeUnlock()
{
    mGate[mD.load()].store(true);
    mLock.release(0);
}

static thread_local int local_d = 0;

void CrmrRwLock::readLock()
{
    int d = mD.load();
    mCount[d].FAI(0,1);

    int dp = mD.load();

    if(d != dp)
    {
        mCount[dp].FAI(0,1);
        d = mD.load();

        if(mCount[1-d].FAD(0,1) == std::pair<uint32_t,uint32_t>(1,1))
        {
            mPermit[1-d].store(true);
        }
    }

    while(!mGate[d].load())
    {
        std::this_thread::yield();
    }

    local_d = d;
}

void CrmrRwLock::readUnlock()
{
    mExitCount.FAI(0,1);

    if(mCount[local_d].FAD(0,1) == std::pair<uint32_t,uint32_t>(1,1))
    {
        mPermit[local_d].store(true);
    }

    if(mExitCount.FAD(0,1) == std::pair<uint32_t,uint32_t>(1,1))
    {
        mExitPermit.store(true);
    }

}

