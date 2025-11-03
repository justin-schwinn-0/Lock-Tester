#ifndef BAKERS_LAMPORT_LOCK_H
#define BAKERS_LAMPORT_LOCK_H

#include <vector>
#include <cstdint>
#include <atomic>

#include "Flag.h"

class BakersLamportLock 
{
public:
    explicit BakersLamportLock(int size);
    ~BakersLamportLock();

    BakersLamportLock(const BakersLamportLock&) = delete;
    BakersLamportLock(const BakersLamportLock&&) = delete;
    BakersLamportLock& operator=(BakersLamportLock&) = delete;
    BakersLamportLock& operator=(BakersLamportLock&&) = delete;
    
    void aquire(uint32_t me);
    void release(uint32_t me);

    bool tokenTest(int i, int me);
private:
    std::vector<Flag> flags;
    std::vector<PaddedAtomicInt> tokens;

};

#endif
