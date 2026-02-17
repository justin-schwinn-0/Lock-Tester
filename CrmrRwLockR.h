#pragma once

#include <vector>
#include <cstdint>
#include <atomic>

#include "McsLock.h"

class CrmrRwLockR
{
public:
    CrmrRwLockR();
    ~CrmrRwLockR();

    CrmrRwLockR(const CrmrRwLockR&) = delete;
    CrmrRwLockR(const CrmrRwLockR&&) = delete;
    CrmrRwLockR& operator=(CrmrRwLockR&) = delete;
    CrmrRwLockR& operator=(CrmrRwLockR&&) = delete;
    
    void readLock();

    void readUnlock();

    void writeLock();

    void writeUnlock();

    void promote();
    uint64_t getPid();

private:
    std::atomic<uint64_t> mD;
    std::atomic<bool> mGate[2];
    std::atomic<bool> mPermit;
    std::atomic<uint64_t> mX;
    std::atomic<uint64_t> mC;

    McsLock lock;

    static constexpr uint64_t X_TRUE = UINT64_MAX;
};
