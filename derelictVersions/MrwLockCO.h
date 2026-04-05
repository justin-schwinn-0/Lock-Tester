#pragma once

#include <vector>
#include <cstdint>
#include <atomic>

#include <functional>


struct mrwco_qnode
{
    // cap of 2^31, bit 31 is for locked
    alignas(64) std::atomic<uint32_t> count;
    alignas(64) std::atomic<bool> locked;
    std::atomic<mrwco_qnode*> next;
};

class MrwLockCO 
{
public:
    explicit MrwLockCO();
    ~MrwLockCO();

    MrwLockCO(const MrwLockCO&) = delete;
    MrwLockCO(const MrwLockCO&&) = delete;
    MrwLockCO& operator=(MrwLockCO&) = delete;
    MrwLockCO& operator=(MrwLockCO&&) = delete;
    
    void readLock();
    void readUnlock();
    void writeLock();
    void writeUnlock();

    void performAquire(mrwco_qnode* node,uint32_t count);
    void performRelease(mrwco_qnode* node);

    inline bool isLocked(uint32_t counter)
    {
        return (counter & 0x80000000) > 0; 
    }

    inline void setLocked(mrwco_qnode* node, bool set)
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

    inline void resetNode(mrwco_qnode* node,uint32_t count = 0)
    {
        node->count.store((count | LAST_BIT_MASK));
        node->next.store(nullptr);
        node->locked.store(true);
    }

    inline bool spin(mrwco_qnode* node)
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

    void setCohortRelease(std::function<void()> cf)
    {
        mCohortRelease = cf;
    }

    void print();

private:
    std::atomic<mrwco_qnode*> mTail;
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

