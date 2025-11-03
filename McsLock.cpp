#include "McsLock.h"

#include <thread>

static thread_local mcs_qnode mine; 

McsLock::McsLock(int size) :
    mTail(nullptr)
{
}

McsLock::~McsLock()
{
}

void McsLock::aquire(uint32_t me)
{ 
    mcs_qnode* pred = mTail.exchange(&mine);

    if(pred)
    {
        mine.locked= true;
        pred->next.store(&mine);

        while(mine.locked)
        {
            std::this_thread::yield();
        }
    }
}

void McsLock::release(uint32_t me)
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
            std::this_thread::yield();
        }
    }

    mcs_qnode* myNext = mine.next.load();
    myNext->locked = false;
    mine.next.store(nullptr);
}
