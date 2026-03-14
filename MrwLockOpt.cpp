#include "MrwLockOpt.h"

#include <thread>
#include <cstdio>

static thread_local mrwo_qnode mine[2];
static thread_local mrwo_qnode* myTarget;
static thread_local int cur = 0;

uint32_t MrwLockOpt::SEARCH_LIMIT = 5;
uint32_t MrwLockOpt::CAS_LIMIT = 5;

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

    printf("Avg readers pernode %f\n",avgReaders);
    */
}

void MrwLockOpt::writeLock()
{
    myTarget = nullptr;
    cur = 1 - cur;
    performAquire(&mine[cur],0);
}

void MrwLockOpt::writeUnlock()
{
    performRelease(&mine[cur]);
}

void MrwLockOpt::readLock()
{
    bool makeOwnNode = false;
    int searchAttempts = 0;
    mrwo_qnode* lastPointer = nullptr;

    while(searchAttempts < SEARCH_LIMIT)
    {
        // outer loop reads mTail, searching for a node to join
        searchAttempts++;
        lastPointer = myTarget;
        myTarget = mTail.load();

        if(myTarget)
        {
            uint32_t unmaskedCount = myTarget->count.load();
            uint32_t newCount = unmaskedCount+ 1;

            if(!readerCanJoin(unmaskedCount))
            {
                if(myTarget == lastPointer)
                {
                    break;
                }
                continue;
            }

            uint32_t casAttempts = 0;
            while(casAttempts < CAS_LIMIT)
            {
                // inner loop attempts to join a node if one is found
                casAttempts++;
                if(myTarget->count.compare_exchange_strong(unmaskedCount,newCount))
                {
                    // joined successfully, spin on target
                    while(spin(myTarget)) {}
                    return;
                }
                else
                {
                    // failed to join
                    // check if we can join on new value 
                    if(readerCanJoin(unmaskedCount))
                    {
                        // we can still try and join,
                        // set up newCount for next attempt
                        newCount = unmaskedCount + 1;
                    }
                    else
                    {
                        // Abandon attempting to join this node,
                        // we can not join it anymore
                        searchAttempts++;
                        break;
                    }
                }
            }
        }
        else
        {
            if(lastPointer == nullptr)
            {
                break;
            }
        }
    }

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
        }
    }

    setLocked(next,false);
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
