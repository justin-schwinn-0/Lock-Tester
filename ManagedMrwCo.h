#pragma once

#include <vector>
#include <cstdint>
#include <atomic>

#include <functional>


struct mmrwco_qnode
{
    // cap of 2^31, bit 31 is for locked
    alignas(64) std::atomic<uint32_t> count;
    alignas(64) std::atomic<bool> locked;
    alignas(64) std::atomic<mmrwco_qnode*> next;
};

struct qnode_set
{
   mmrwco_qnode node;
   mmrwco_qnode* myTarget;

   qnode_set(){}
};

class ManagedMrwCo 
{
public:
    explicit ManagedMrwCo();
    ~ManagedMrwCo();

    ManagedMrwCo(const ManagedMrwCo&) = delete;
    ManagedMrwCo(const ManagedMrwCo&&) = delete;
    ManagedMrwCo& operator=(ManagedMrwCo&) = delete;
    ManagedMrwCo& operator=(ManagedMrwCo&&) = delete;
    

    void readLock();
    void readUnlock();
    void writeLock();
    void writeUnlock();

    inline void readLock(qnode_set* set)
    {
        int searchAttempts = 0;
        mmrwco_qnode* lastPointer = nullptr;
        while(searchAttempts < SEARCH_LIMIT)
        {
            searchAttempts++;
            // track previous target
            lastPointer = set->myTarget;
            // outer loop reads mTail, searching for a node to join
            set->myTarget = mTail.load();
            if(set->myTarget == lastPointer)
            { //if myTarget is the same as the last pointer 
              //just enqueue your own node
                break;
            }
            if(set->myTarget)
            {
                uint32_t unmaskedCount = -set->myTarget->count.load();
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
                    if(set->myTarget->count.compare_exchange_strong(unmaskedCount,newCount))
                    {
                        // joined successfully, spin on target
                        while(spin(set->myTarget)) {}
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
        set->myTarget = &set->node; 
        performAquire(&set->node,1);
    }

    inline void readUnlock(qnode_set* set)
    {
        uint32_t curCount = set->myTarget->count.fetch_sub(1);
        if(curCount == 1)
        {
            performRelease(set->myTarget);
        }
    }

    inline void writeLock(qnode_set* set)
    {
        set->myTarget = nullptr;
        performAquire(&set->node,0);
    }

    inline void writeUnlock(qnode_set* set)
    {
        performRelease(&set->node);
    }

    void performAquire(mmrwco_qnode* node,uint32_t count);
    void performRelease(mmrwco_qnode* node);

    inline bool isLocked(uint32_t counter)
    {
        return (counter & 0x80000000) > 0; 
    }

    inline void setLocked(mmrwco_qnode* node, bool set)
    {
        if(set)
        {
            node->count.fetch_or(LAST_BIT_MASK);
        }
        else
        {
            node->count.fetch_and(~LAST_BIT_MASK);
/*
            if(node->count > 0)
            {
                totalReaders.fetch_add(node->count.load());
                totalReads.fetch_add(1);
            }*/
        }

        node->locked.store(set);
    }

    inline void resetNode(mmrwco_qnode* node,uint32_t count = 0)
    {
        node->count.store((count | LAST_BIT_MASK));
        node->next.store(nullptr);
        node->locked.store(true);
    }

    inline bool spin(mmrwco_qnode* node)
    {
        return node->locked;
        //return isLocked(node->count.load());
    }

    inline bool readerCanJoin(uint32_t count)
    {
        return count >= LOCKED_READING_START_MASK;
    }

    static void setNodeSearchLimit(uint32_t val)
    {
        SEARCH_LIMIT = val;
    }

    static void setCasAttemptLimit(uint32_t val)
    {
        CAS_LIMIT = val;
    }

    qnode_set* getNodeSet();

private:
    std::atomic<mmrwco_qnode*> mTail;
    std::function<void()> mCohortRelease;

    std::atomic<uint64_t> successes;
    std::atomic<uint64_t> misses;
    std::atomic<uint64_t> lockedOut;
    std::atomic<uint64_t> totalReaders;
    std::atomic<uint64_t> totalReads;


    static constexpr uint32_t LOCKED_READING_START_MASK = 0x80000001;
    static constexpr uint32_t LAST_BIT_MASK = 1 << 31; 
    static uint32_t SEARCH_LIMIT;
    static uint32_t CAS_LIMIT;
};

