#pragma once

#include <vector>
#include <cstdint>
#include <atomic>
#include <functional>

#include "McsLock.h"
#include "Counter.h"


class CrmrRwLockCo
{
public:
    CrmrRwLockCo();
    ~CrmrRwLockCo();

    CrmrRwLockCo(const CrmrRwLockCo&) = delete;
    CrmrRwLockCo(const CrmrRwLockCo&&) = delete;
    CrmrRwLockCo& operator=(CrmrRwLockCo&) = delete;
    CrmrRwLockCo& operator=(CrmrRwLockCo&&) = delete;
    
    void readLock();

    void readUnlock();

    void writeLock();

    void writeUnlock();

    void setCohortRelease(std::function<void()> cf)
    {
        mCohortRelease = cf;
    }

private:
    alignas(64) std::atomic<int> mD;
    alignas(64) std::atomic<bool> mExitPermit;
    alignas(64) std::atomic<bool> mPermit[2];
    alignas(64) std::atomic<bool> mGate[2];
    alignas(64) Counter mCount[2];
    alignas(64) Counter mExitCount;
    std::function<void()> mCohortRelease;
    
    McsLock mLock;

};
