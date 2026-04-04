#include "McsLock.h"

#include <thread>

static thread_local mcs_qnode mine; 

McsLock::McsLock(int size) :
    mTail(nullptr)
{
}

McsLock::McsLock() :
    mTail(nullptr)
{
}

McsLock::~McsLock()
{
}

void McsLock::aquire(uint32_t me)
{ 
    lock();
}

void McsLock::release(uint32_t me)
{
    unlock();
}

void McsLock::lock()
{
    mcs_qnode* pred = mTail.exchange(&mine);

    if(pred)
    {
        mine.locked.store(true);
        pred->next.store(&mine);

        while(mine.locked.load())
        {
            //std::this_thread::yield();
        }
    }
}

void McsLock::unlock()
{
    if(mine.next == nullptr)
    {
        mcs_qnode* tmp = &mine;
        if(mTail.compare_exchange_strong(tmp,static_cast<mcs_qnode*>(nullptr)))
        {
            return;
        }
        while(!mine.next) 
        {
            //std::this_thread::yield();
        }
    }

    mcs_qnode* myNext = mine.next.load();
    myNext->locked.store(false);
    mine.next.store(nullptr);
}
