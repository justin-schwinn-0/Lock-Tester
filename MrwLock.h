#pragma once

#include <vector>
#include <cstdint>
#include <atomic>

struct mrw_qnode
{
    // cap of 2^31, bit 31 is for locked
    std::atomic<uint32_t> count;
    std::atomic<mrw_qnode*> next;
};

class MrwLock 
{
public:
    explicit MrwLock();
    ~MrwLock();

    MrwLock(const MrwLock&) = delete;
    MrwLock(const MrwLock&&) = delete;
    MrwLock& operator=(MrwLock&) = delete;
    MrwLock& operator=(MrwLock&&) = delete;
    
    void readLock();
    void readUnlock();
    void writeLock();
    void writeUnlock();

    void performAquire(mrw_qnode* node);
    void performRelease(mrw_qnode* node);

    bool isLocked(uint32_t counter);
    void setLocked(mrw_qnode* node, bool set);
    void resetNode(mrw_qnode* node);

    inline bool spin(mrw_qnode* node)
    {
        return isLocked(node->count.load());
        //return isLocked(node->count.load());
    }

    void print();

private:
    std::atomic<mrw_qnode*> mTail;

    static constexpr uint32_t LOCKED_READING_START_MASK = 0x80000001;
    static constexpr uint32_t LAST_BIT_MASK = 1 << 31; 
};
