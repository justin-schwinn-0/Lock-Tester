#include "MrwLockCO.h"

#include <thread>
#include <cstdio>


static thread_local mrwco_qnode mine[2];
static thread_local mrwco_qnode* myTarget;
static thread_local int cur = 0;

uint32_t MrwLockCO::SEARCH_LIMIT = 5;
uint32_t MrwLockCO::CAS_LIMIT = 5;

MrwLockCO::MrwLockCO():
    mTail(nullptr)
{
}
MrwLockCO::~MrwLockCO()
{
    /*
    printf("Lock successes %lu times\n",successes.load());
    printf("Lock Missed    %lu times\n",misses.load());
    printf("Locked out     %lu times\n",lockedOut.load());

    double avgReaders = static_cast<double>(totalReaders.load()) / totalReads.load();

    printf("Avg readers pernode %f\n",avgReaders);
    */
}

void MrwLockCO::writeLock()
{
    myTarget = nullptr;
    cur = 1 - cur;
    performAquire(&mine[cur],0);
}

void MrwLockCO::writeUnlock()
{
    performRelease(&mine[cur]);
}

void MrwLockCO::readLock()
{
    int searchAttempts = 0;
    mrwco_qnode* lastPointer = nullptr;
    while(searchAttempts < SEARCH_LIMIT)
    {
        searchAttempts++;
        // track previous target
        lastPointer = myTarget;
        // outer loop reads mTail, searching for a node to join
        myTarget = mTail.load();
        if(myTarget == lastPointer)
        { //if myTarget is the same as the last pointer 
          //just enqueue your own node
            break;
        }
        if(myTarget)
        {
            uint32_t unmaskedCount = myTarget->count.load();
            uint32_t newCount = unmaskedCount+ 1;
            if(!readerCanJoin(unmaskedCount))
            {
                continue;
            }
            uint32_t casAttempts = 0;
            while(casAttempts < CAS_LIMIT)
            {
                // inner loop attempts to join the node 
                casAttempts++;
                if(myTarget->count.compare_exchange_strong(unmaskedCount,newCount))
                {
                    mCohortRelease();
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
                    else // readerCanJoin returned false
                    {
                        // Abandon attempting to join this node,
                        break;
                    }
                }
            }
        }
        else
        {
            break;
        }
    }
    cur = 1 - cur;
    myTarget = &mine[cur];
    performAquire(&mine[cur],1);
}

void MrwLockCO::readUnlock()
{
    uint32_t curCount = myTarget->count.fetch_sub(1);
    if(curCount == 1)
    {
        performRelease(myTarget);
    }
}

void MrwLockCO::performAquire(mrwco_qnode* node,uint32_t count)
{
    resetNode(node,count);
    mrwco_qnode* pred = mTail.exchange(node);
    mCohortRelease();

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

void MrwLockCO::performRelease(mrwco_qnode* node)
{
    mrwco_qnode* next = node->next.load();
    if(next == nullptr)
    {
        mrwco_qnode* tmp = node;
        if(mTail.compare_exchange_strong(tmp,static_cast<mrwco_qnode*>(nullptr)))
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
void MrwLockCO::print()
{
    mrwco_qnode* node = &mine[cur];
    while(node)
    {
        printf("{%x}->",node->count.load());
        node = node->next.load();
    }
    printf("\n");

}
