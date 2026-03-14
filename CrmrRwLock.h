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
    alignas(64) std::atomic<int> mD;
    alignas(64) std::atomic<bool> mExitPermit;
    alignas(64) std::atomic<bool> mPermit[2];
    alignas(64) std::atomic<bool> mGate[2];
    alignas(64) Counter mCount[2];
    alignas(64) Counter mExitCount;

    McsLock mLock;

};
