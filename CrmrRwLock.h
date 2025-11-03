#pragma once

#include <vector>
#include <cstdint>
#include <atomic>

#include "McsLock.h"
#include "Counter.h"


class CrmrRwLock
{
public:
    CrmrRwLock();
    ~CrmrRwLock();

    CrmrRwLock(const CrmrRwLock&) = delete;
    CrmrRwLock(const CrmrRwLock&&) = delete;
    CrmrRwLock& operator=(CrmrRwLock&) = delete;
    CrmrRwLock& operator=(CrmrRwLock&&) = delete;
    
    void readLock();

    void readUnlock();

    void writeLock();

    void writeUnlock();

private:
    std::atomic<int> mD;
    std::atomic<bool> mExitPermit;
    std::atomic<bool> mPermit[2];
    std::atomic<bool> mGate[2];
    Counter mCount[2];
    Counter mExitCount;

    McsLock mLock;

};
