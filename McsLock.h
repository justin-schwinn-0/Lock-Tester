#ifndef MCS_LOCK_H
#define MCS_LOCK_H

#include <vector>
#include <cstdint>
#include <atomic>

struct mcs_qnode
{
    alignas(64) std::atomic<bool> locked;
    alignas(64) std::atomic<mcs_qnode*> next;
};

class McsLock 
{
public:
    explicit McsLock(int size);
    explicit McsLock();
    ~McsLock();

    McsLock(const McsLock&) = delete;
    McsLock(const McsLock&&) = delete;
    McsLock& operator=(McsLock&) = delete;
    McsLock& operator=(McsLock&&) = delete;
    
    void aquire(uint32_t me);
    void release(uint32_t me);

    void lock();

    void unlock();
private:
    std::atomic<mcs_qnode*> mTail;
};


#endif
