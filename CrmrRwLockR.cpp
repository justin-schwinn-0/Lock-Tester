#include "CrmrRwLockR.h"
#include "Utils.h"

#include <thread>
#include <string>
#include <sstream>

CrmrRwLockR::CrmrRwLockR():
    mD(0),
    mGate{{true},{false}},
    mPermit(true),
    mX(0),
    mC(0),
    lock(McsLock(0))
{
}

CrmrRwLockR::~CrmrRwLockR()
{
}

void CrmrRwLockR::writeLock()
{
    lock.aquire(0);
    mD.fetch_xor(0x1);
    mPermit.store(false);
    promote();

    while(!mPermit.load())
    {
        std::this_thread::yield();
    }
}

void CrmrRwLockR::writeUnlock()
{
    int d = static_cast<int>(mD.load());
    mGate[1 - d].store(false);
    mGate[d].store(true);

    mX.store(getPid());
    lock.release(0);
}

void CrmrRwLockR::readLock()
{
    mC.fetch_add(1);

    int d = static_cast<int>(mD.load());
    uint64_t x = mX.load();

    if(x != X_TRUE)
    {
        mX.compare_exchange_strong(x,getPid());
    }

    if(mX.load() == X_TRUE)
    {
        while(!mGate[d].load())
        {
            std::this_thread::yield();
        }
    }
}

void CrmrRwLockR::readUnlock()
{
    mC.fetch_add(-1);
    promote();
}


void CrmrRwLockR::promote()
{
    uint64_t x = mX.load();

    if(x != X_TRUE)
    {
        if(mX.compare_exchange_strong(x,getPid()))
        {
            if(!mPermit)
            {
                if(mC.load() == 0)
                {
                    uint64_t tmp = getPid();
                    if(mX.compare_exchange_strong(tmp,X_TRUE))
                    {
                        mPermit.store(true);
                    }
                }
            }
        }
    }
}

uint64_t CrmrRwLockR::getPid()
{
    static thread_local uint64_t pid = 0;
    if(pid == 0)
    {
        static std::atomic<int> nextPid = 2;
        pid = nextPid.fetch_add(1);
    }
    return pid;
}
