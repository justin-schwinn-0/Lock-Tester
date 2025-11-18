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
}

void MrwLockOpt::writeLock()
{
    myTarget = nullptr;
    cur = 1 - cur;
    resetNode(&mine[cur]);
    performAquire(&mine[cur]);
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

            if(curCount == 0 || !isLocked(unmaskedCount))
            {
                makeOwnNode = true;
                continue;
            }
            while(true)
            {
                if(myTarget->count.compare_exchange_strong(unmaskedCount,newCount))
                {
                    // cas successful, spin on target
                    while(spin(myTarget))
                    {
                        std::this_thread::yield();
                    }
                    return;
                }
                else
                {
                    // cas failed
                    // count inc or is no longer locked
                    if(isLocked(unmaskedCount))
                    {
                        newCount = unmaskedCount + 1;
                    }
                    else
                    {
                        // go make own node
                        makeOwnNode = true;
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

    resetNode(&mine[cur]);

    mine[cur].count.fetch_add(1);
    performAquire(&mine[cur]);

}

void MrwLockOpt::readUnlock()
{
    uint32_t curCount = myTarget->count.fetch_sub(1);
    if(curCount == 1)
    {
        performRelease(myTarget);
    }
}

void MrwLockOpt::performAquire(mrwo_qnode* node)
{
    mrwo_qnode* pred = mTail.exchange(node);

    if(pred)
    {
        setLocked(node,true);
        pred->next.store(node);

        // while locked==true
        while(spin(node))
        {
            std::this_thread::yield();
        }
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
            std::this_thread::yield();
        }
    }

    if(next)
    {
        setLocked(next,false);
    }
}

bool MrwLockOpt::isLocked(uint32_t counter)
{
    return (counter & 0x80000000) > 0; 
}

void MrwLockOpt::setLocked(mrwo_qnode* node, bool set)
{
    if(set)
    {
        node->count.fetch_or(LAST_BIT_MASK);
    }
    else
    {
        node->count.fetch_and(~LAST_BIT_MASK);
    }

    node->locked = set;
}

void MrwLockOpt::resetNode(mrwo_qnode* node)
{
    node->count.store(0);
    node->next.store(nullptr);
    setLocked(node,true);
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
