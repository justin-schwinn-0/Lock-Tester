#ifndef FETCH_AND_INC_LOCK_H
#define FETCH_AND_INC_LOCK_H

#include <cstdint>
#include <atomic>

#include "Flag.h"

class FetchAndIncLock 
{
public:
    explicit FetchAndIncLock(int size);
    ~FetchAndIncLock();

    FetchAndIncLock(const FetchAndIncLock&) = delete;
    FetchAndIncLock(const FetchAndIncLock&&) = delete;
    FetchAndIncLock& operator=(FetchAndIncLock&) = delete;
    FetchAndIncLock& operator=(FetchAndIncLock&&) = delete;
    
    void aquire(uint32_t me);
    void release(uint32_t me);
private:
    std::atomic<int> token;
    int turn;

};

#endif
