#include "MrwLockOpt.h"

#include <thread>
#include <cstdio>

// changes are now frozen!
// nvm added concurrent entering!
// nvm removed concurrent entering!

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

    //printf("total nodes %lu\n",totalNodes.load());
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
    int searchAttempts = 0;
    mrwo_qnode* lastPointer = nullptr;
    while(searchAttempts < SEARCH_LIMIT)
    {
        searchAttempts++;
        // track previous target
        lastPointer = myTarget;
        // outer loop reads mTail, searching for a node to join
        myTarget = mTail.load();
        if(myTarget == lastPointer)
        { 
            //if myTarget is the same as the last pointer 
            //just enqueue your own node
            break;
        }
        if(myTarget)
        {
            uint32_t count = myTarget->count.load();
            if(!readerCanJoin(count))
            {
                // can't join this node,
                // move on to next one
                continue;
            }
            
            bool joined = tryJoinNode(myTarget,count);

            if(joined)
            {
                return;
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

    //totalNodes.fetch_add(1);
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
