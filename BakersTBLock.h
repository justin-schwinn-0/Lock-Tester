#ifndef BAKERS_TB_LOCK_H
#define BAKERS_TB_LOCK_H

#include <vector>
#include <cstdint>
#include <atomic>

#include "Flag.h"

class BakersTBLock 
{
public:
    explicit BakersTBLock(int size);
    ~BakersTBLock();

    BakersTBLock(const BakersTBLock&) = delete;
    BakersTBLock(const BakersTBLock&&) = delete;
    BakersTBLock& operator=(BakersTBLock&) = delete;
    BakersTBLock& operator=(BakersTBLock&&) = delete;
    
    void aquire(uint32_t me);
    void release(uint32_t me);

    bool tokenTest(int i, int me);
private:
    std::vector<Flag> flags;
    std::vector<PaddedAtomicInt> tokens;

};

#endif
