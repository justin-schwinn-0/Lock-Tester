#include "MrwLockOpt.h"

#include <thread>
#include <cstdio>

static thread_local mrwo_qnode mine[2];
static thread_local mrwo_qnode* myTarget;
static thread_local int cur = 0;

MrwLockOpt::MrwLockOpt():
    mTail(nullptr)
{
}
MrwLockOpt::~MrwLockOpt()
{
    /*
    printf("Lock successes %lu times\n",successes.load());
    printf("Lock Missed    %lu times\n",misses.load());
    printf("Locked out     %lu times\n",lockedOut.load());

    double avgReaders = static_cast<double>(totalReaders.load()) / totalReads.load();

    printf("Average readers per read node %f times\n",avgReaders);
    */
}

void MrwLockOpt::writeLock()
{
    myTarget = nullptr;
    cur = 1 - cur;
    //resetNode(&mine[cur]);
    performAquire(&mine[cur],0);
}

void MrwLockOpt::writeUnlock()
{
    performRelease(&mine[cur]);
}

void MrwLockOpt::readLock()
{
    bool makeOwnNode = false;
    do
    {
        myTarget = mTail.load();

        if(myTarget)
        {
            uint32_t unmaskedCount = myTarget->count.load();
            uint32_t curCount = unmaskedCount & (~LAST_BIT_MASK);
            uint32_t newCount = unmaskedCount+ 1;

            if(!readerCanJoin(unmaskedCount))
            {
                makeOwnNode = true;
                continue;
            }
            while(true)
            {
                if(myTarget->count.compare_exchange_strong(unmaskedCount,newCount))
                {
                    //successes.fetch_add(1);
                    // cas successful, spin on target
                    while(spin(myTarget)) {}
                    return;
                }
                else
                {
                    // cas failed
                    // count inc or is no longer locked
                    if(readerCanJoin(unmaskedCount))
                    {
                        newCount = unmaskedCount + 1;
                        //misses.fetch_add(1);
                    }
                    else
                    {
                        // go make own node
                        makeOwnNode = true;
                        //lockedOut.fetch_add(1);
                        break;
                    }
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

    performAquire(&mine[cur],1);
}

void MrwLockOpt::readUnlock()
{
    uint32_t curCount = myTarget->count.fetch_sub(1);
    if(curCount == 1)
    {
        performRelease(myTarget);
    }
}

void MrwLockOpt::performAquire(mrwo_qnode* node,uint32_t count)
{
    resetNode(node,count);
    mrwo_qnode* pred = mTail.exchange(node);

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

void MrwLockOpt::performRelease(mrwo_qnode* node)
{
    mrwo_qnode* next = node->next.load();
    if(next == nullptr)
    {
        mrwo_qnode* tmp = node;
        if(mTail.compare_exchange_strong(tmp,static_cast<mrwo_qnode*>(nullptr)))
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
void MrwLockOpt::print()
{
    mrwo_qnode* node = &mine[cur];
    while(node)
    {
        printf("{%x}->",node->count.load());
        node = node->next.load();
    }
    printf("\n");

}
