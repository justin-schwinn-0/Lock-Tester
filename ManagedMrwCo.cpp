#include "ManagedMrwCo.h"

#include <thread>
#include <cstdio>
#include <unordered_map>
#include <map>

static thread_local std::unordered_map<ManagedMrwCo*, qnode_set*> nodeMap;


uint32_t ManagedMrwCo::SEARCH_LIMIT = 5;
uint32_t ManagedMrwCo::CAS_LIMIT = 5;

ManagedMrwCo::ManagedMrwCo():
    mTail(nullptr)
{
}
ManagedMrwCo::~ManagedMrwCo()
{
    /*
    printf("Lock successes %lu times\n",successes.load());
    printf("Lock Missed    %lu times\n",misses.load());
    printf("Locked out     %lu times\n",lockedOut.load());

    double avgReaders = static_cast<double>(totalReaders.load()) / totalReads.load();

    printf("Avg readers pernode %f\n",avgReaders);
    */
}

void ManagedMrwCo::readLock()
{
    qnode_set* set = getNodeSet();
    readLock(set);
}

void ManagedMrwCo::readUnlock()
{
    qnode_set* set = getNodeSet();
    readUnlock(set);
}

void ManagedMrwCo::writeLock()
{
    qnode_set* set = getNodeSet();
    writeLock(set);
}

void ManagedMrwCo::writeUnlock()
{
    qnode_set* set = getNodeSet();
    writeUnlock(set);
}

qnode_set* ManagedMrwCo::getNodeSet()
{
    auto nodeSet = nodeMap[this];
    if(!nodeSet)
    {
        nodeMap[this] = new qnode_set();
        nodeSet = nodeMap[this];
    }
    return nodeSet;
}
void ManagedMrwCo::performAquire(mmrwco_qnode* node,uint32_t count)
{
    resetNode(node,count);
    mmrwco_qnode* pred = mTail.exchange(node);

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

void ManagedMrwCo::performRelease(mmrwco_qnode* node)
{
    mmrwco_qnode* next = node->next.load();
    if(next == nullptr)
    {
        mmrwco_qnode* tmp = node;
        if(mTail.compare_exchange_strong(tmp,static_cast<mmrwco_qnode*>(nullptr)))
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
