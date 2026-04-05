#pragma once

#include <vector>
#include <cstdint>
#include <atomic>


struct mrwsc_qnode
{
    alignas(64) std::atomic<uint32_t> count;
    alignas(64) bool locked;
    std::atomic<mrwsc_qnode*> next;
};

class MrwLockSc 
{
public:
    explicit MrwLockSc();
    ~MrwLockSc();

    MrwLockSc(const MrwLockSc&) = delete;
    MrwLockSc(const MrwLockSc&&) = delete;
    MrwLockSc& operator=(MrwLockSc&) = delete;
    MrwLockSc& operator=(MrwLockSc&&) = delete;
    
    void readLock();
    void readUnlock();
    void writeLock();
    void writeUnlock();

    void performAquire(mrwsc_qnode* node);
    void performRelease(mrwsc_qnode* node);

    inline void setLocked(mrwsc_qnode* node, bool set)
    {
        /*if(node->count > 0)
        {
            totalReaders.fetch_add(node->count.load());
            totalReads.fetch_add(1);
        }*/

        node->locked = set;
    }

    inline void resetNode(mrwsc_qnode* node)
    {
        node->count.store(0);
        node->next.store(nullptr);
        setLocked(node,true);
    }

    inline bool spin(mrwsc_qnode* node)
    {
        return node->locked;
    }

    void print();

private:
    std::atomic<mrwsc_qnode*> mTail;

    std::atomic<uint64_t> successes;
    std::atomic<uint64_t> misses;
    std::atomic<uint64_t> lockedOut;
    std::atomic<uint64_t> totalReaders;
    std::atomic<uint64_t> totalReads;

};

