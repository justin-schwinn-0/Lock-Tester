#include "MrwLock.h"

#include <thread>
#include <cstdio>

static thread_local mrw_qnode mine[2];
static thread_local mrw_qnode* myTarget;
static thread_local int cur = 0;

MrwLock::MrwLock():
    mTail(nullptr)
{
}
MrwLock::~MrwLock()
{
}

void MrwLock::writeLock()
{
    myTarget = nullptr;
    cur = 1 - cur;
    resetNode(&mine[cur]);
    performAquire(&mine[cur]);
}

void MrwLock::writeUnlock()
{
    performRelease(&mine[cur]);
}

void MrwLock::readLock()
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
                    while(isLocked(myTarget->count.load()))
                    {
                        std::this_thread::yield();
                    }
                    return;
                }
                else
                {
                    // cas failed
                    // count inc or is no longer locked
                    if(isLocked(curCount))
                    {
                        newCount = curCount+1;
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

void MrwLock::readUnlock()
{
    uint32_t curCount = myTarget->count.fetch_sub(1);
    if(curCount == 1)
    {
        performRelease(myTarget);
    }
}

void MrwLock::performAquire(mrw_qnode* node)
{
    mrw_qnode* pred = mTail.exchange(node);

    if(pred)
    {
        setLocked(node,true);
        pred->next.store(node);

        // while locked==true
        while(isLocked(node->count.load()))
        {
            std::this_thread::yield();
        }
    }
    else
    {
        setLocked(node,false);
    }
}

void MrwLock::performRelease(mrw_qnode* node)
{
    mrw_qnode* next = node->next.load();
    if(next == nullptr)
    {
        mrw_qnode* tmp = node;
        if(mTail.compare_exchange_strong(tmp,static_cast<mrw_qnode*>(nullptr)))
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

bool MrwLock::isLocked(uint32_t counter)
{
    return (counter & 0x80000000) > 0; 
}

void MrwLock::setLocked(mrw_qnode* node, bool set)
{
    if(set)
    {
        node->count.fetch_or(LAST_BIT_MASK);
    }
    else
    {
        node->count.fetch_and(~LAST_BIT_MASK);
    }
}

void MrwLock::resetNode(mrw_qnode* node)
{
    node->count.store(0);
    node->next.store(nullptr);
    setLocked(node,true);
}

void MrwLock::print()
{
    mrw_qnode* node = &mine[cur];
    while(node)
    {
        printf("{%x}->",node->count.load());
        node = node->next.load();
    }
    printf("\n");

}
