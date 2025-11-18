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

    bool isLocked(uint32_t counter);
    void setLocked(mrwo_qnode* node, bool set);
    void resetNode(mrwo_qnode* node);

    inline bool spin(mrwo_qnode* node)
    {
        return node->locked;
        //return isLocked(node->count.load());
    }

    void print();

private:
    std::atomic<mrwo_qnode*> mTail;

    static constexpr uint32_t LOCKED_READING_START_MASK = 0x80000001;
    static constexpr uint32_t LAST_BIT_MASK = 1 << 31; 
};
