#ifndef MCS_LOCK_H
#define MCS_LOCK_H

#include <vector>
#include <cstdint>
#include <atomic>

struct mcs_qnode
{
    alignas(64) bool locked;
    alignas(64) std::atomic<mcs_qnode*> next;
};

class McsLock 
{
public:
    explicit McsLock(int size);
    ~McsLock();

    McsLock(const McsLock&) = delete;
    McsLock(const McsLock&&) = delete;
    McsLock& operator=(McsLock&) = delete;
    McsLock& operator=(McsLock&&) = delete;
    
    void aquire(uint32_t me);
    void release(uint32_t me);
private:
    std::atomic<mcs_qnode*> mTail;
};


#endif
