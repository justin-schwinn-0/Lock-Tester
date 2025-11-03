#ifndef PETERSON_LOCK_H
#define PETERSON_LOCK_H

#include <vector>
#include <cstdint>
#include <atomic>

#include "Flag.h"

class PetersonLock
{
public:
    PetersonLock(int doesNothing = 0);
    ~PetersonLock();

    PetersonLock(const PetersonLock&) = delete;
    PetersonLock(const PetersonLock&&) = delete;
    PetersonLock& operator=(PetersonLock&) = delete;
    PetersonLock& operator=(PetersonLock&&) = delete;

    void aquire(uint32_t me);
    void release(uint32_t me);

private:
    std::vector<Flag> flags;
    std::atomic<int> victim;
};
#endif
