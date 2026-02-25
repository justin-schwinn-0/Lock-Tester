#include "MrwLockSc.h"

#include <thread>
#include <cstdio>

static thread_local mrwsc_qnode mine[2];
static thread_local mrwsc_qnode* myTarget;
static thread_local int cur = 0;

MrwLockSc::MrwLockSc():
    mTail(nullptr)
{
}
MrwLockSc::~MrwLockSc()
{
    /*
    printf("Lock successes %lu times\n",successes.load());
    printf("Lock Missed    %lu times\n",misses.load());
    printf("Locked out     %lu times\n",lockedOut.load());

    double avgReaders = static_cast<double>(totalReaders.load()) / totalReads.load();

    printf("Average readers per read node %f times\n",avgReaders);
    */
}

void MrwLockSc::writeLock()
{
    myTarget = nullptr;
    cur = 1 - cur;
    resetNode(&mine[cur]);
    performAquire(&mine[cur]);
}

void MrwLockSc::writeUnlock()
{
    performRelease(&mine[cur]);
}

void MrwLockSc::readLock()
{
    bool makeOwnNode = false;
    do
    {
        myTarget = mTail.load();

        if(myTarget)
        {
            uint32_t count = myTarget->count.load();

            if(count == 0 || !isLocked())
            {
                // locked or a writer
                makeOwnNode = true;
                continue;
            }
            else
            {
                uint32_t lastCount = myTarget->count.fetch_add(1);
                if(lastCount != 0)
                {
                    // successful join
                    while(spin(myTarget)) {}
                }
                else
                {
                    // inc when another thread is already releasing the node
                    makeOwnNode = true;
                }
            }
        }
        else
        {
            makeOwnNode = true;
        }
    }
    while(!makeOwnNode);

    cur = 1 - cur;
    myTarget = &mine[cur];

    resetNode(&mine[cur]);

    mine[cur].count.fetch_add(1);
    performAquire(&mine[cur]);

}

void MrwLockSc::readUnlock()
{
    uint32_t curCount = myTarget->count.fetch_sub(1);
    if(curCount == 1)
    {
        performRelease(myTarget);
    }
}

void MrwLockSc::performAquire(mrwsc_qnode* node)
{
    mrwsc_qnode* pred = mTail.exchange(node);

    if(pred)
    {
        setLocked(node,true);
        pred->next.store(node);

        while(spin(node)){}
    }
    else
    {
        setLocked(node,false);
    }
}

void MrwLockSc::performRelease(mrwsc_qnode* node)
{
    mrwsc_qnode* next = node->next.load();
    if(next == nullptr)
    {
        mrwsc_qnode* tmp = node;
        if(mTail.compare_exchange_strong(tmp,static_cast<mrwsc_qnode*>(nullptr)))
        {
            return;
        }

        while(!next) 
        {
            next = node->next.load();
            //std::this_thread::yield();
        }
    }

    if(next)
    {
        setLocked(next,false);
    }
}
void MrwLockSc::print()
{
    mrwsc_qnode* node = &mine[cur];
    while(node)
    {
        printf("{%x}->",node->count.load());
        node = node->next.load();
    }
    printf("\n");

}
