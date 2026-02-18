#pragma once

#include <vector>
#include <cstdint>
#include <atomic>


struct mrwo_qnode
{
    // cap of 2^31, bit 31 is for locked
    alignas(64) std::atomic<uint32_t> count;
    alignas(64) bool locked;
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

    void performAquire(mrwo_qnode* node);
    void performRelease(mrwo_qnode* node);

    inline bool isLocked(uint32_t counter)
    {
        return (counter & 0x80000000) > 0; 
    }

    inline void setLocked(mrwo_qnode* node, bool set)
    {
        if(set)
        {
            node->count.fetch_or(LAST_BIT_MASK);
        }
        else
        {
            node->count.fetch_and(~LAST_BIT_MASK);

            /*if(node->count > 0)
            {
                totalReaders.fetch_add(node->count.load());
                totalReads.fetch_add(1);
            }*/
        }

        node->locked = set;
    }

    inline void resetNode(mrwo_qnode* node)
    {
        node->count.store(0);
        node->next.store(nullptr);
        setLocked(node,true);
    }

    inline bool spin(mrwo_qnode* node)
    {
        return node->locked;
        //return isLocked(node->count.load());
    }

    void print();

private:
    std::atomic<mrwo_qnode*> mTail;

    std::atomic<uint64_t> successes;
    std::atomic<uint64_t> misses;
    std::atomic<uint64_t> lockedOut;
    std::atomic<uint64_t> totalReaders;
    std::atomic<uint64_t> totalReads;


    //static constexpr uint32_t LOCKED_READING_START_MASK = 0x80000001;
    static constexpr uint32_t LAST_BIT_MASK = 1 << 31; 
};

