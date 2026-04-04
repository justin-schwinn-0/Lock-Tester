#pragma once

#include <vector>
#include <cstdint>
#include <atomic>


struct mrwo_qnode
{
    // cap of 2^31, bit 31 is for locked
    alignas(64) std::atomic<uint32_t> count;
    alignas(64) std::atomic<bool> locked;
    std::atomic<mrwo_qnode*> next;
};

class MrwLockOpt 
{
public:
    explicit MrwLockOpt();
    ~MrwLockOpt();

    MrwLockOpt(const MrwLockOpt&) = delete;
    MrwLockOpt(const MrwLockOpt&&) = delete;
    MrwLockOpt& operator=(MrwLockOpt&) = delete;
    MrwLockOpt& operator=(MrwLockOpt&&) = delete;
    
    void readLock();
    void readUnlock();
    void writeLock();
    void writeUnlock();

    void performAquire(mrwo_qnode* node,uint32_t count);
    void performRelease(mrwo_qnode* node);

    void setLocked(mrwo_qnode* node, bool set)
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

    void resetNode(mrwo_qnode* node,uint32_t count = 0)
    {
        node->count.store((count | LAST_BIT_MASK));
        node->next.store(nullptr);
        node->locked.store(true);
    }

    bool spin(mrwo_qnode* node)
    {
        return node->locked;
        //return isLocked(node->count.load());
    }

    bool readerCanJoin(uint32_t count)
    {
        // return value of lower 31 bits
        return (count & ~LAST_BIT_MASK) > 0;
    }


    bool tryJoinNode(mrwo_qnode* node, uint32_t expec)
    {

        int casAttempts = 0;
        while(casAttempts < CAS_LIMIT)
        {
            casAttempts++;
            if(node->count.compare_exchange_strong(expec,expec+1))
            {
                // joined successfully, spin on target
                while(spin(node)) {}
                return true;
            }
            else
            {
                // failed to join
                // check if we can join on new value 
                if(readerCanJoin(expec))
                {
                    // we can still try and join,
                    // set up newCount for next attempt
                    expec = expec + 1;
                }
                else 
                {
                    // readerCanJoin returned false
                    // Abandon attempting to join this node,
                    return false;
                }
            }
        }
        // attempts to join exhausted, move on to next node
        return false;
    }

    static void setNodeSearchLimit(uint32_t val)
    {
        SEARCH_LIMIT = val;
    }

    static void setCasAttemptLimit(uint32_t val)
    {
        CAS_LIMIT = val;
    }

    void print();

private:
    std::atomic<mrwo_qnode*> mTail;

    std::atomic<uint64_t> successes;
    std::atomic<uint64_t> misses;
    std::atomic<uint64_t> lockedOut;
    std::atomic<uint64_t> totalReaders;
    std::atomic<uint64_t> totalReads;

    std::atomic<uint64_t> totalNodes;


    static constexpr uint32_t LOCKED_READING_START_MASK = 0x80000001;
    static constexpr uint32_t LAST_BIT_MASK = 1 << 31; 
    static uint32_t SEARCH_LIMIT;
    static uint32_t CAS_LIMIT;
};

