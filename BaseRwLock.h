#pragma once

#include <shared_mutex>

class BaseRwLock
{
public:
    BaseRwLock() {}
    ~BaseRwLock(){}

    BaseRwLock(const BaseRwLock&) = delete;
    BaseRwLock(const BaseRwLock&&) = delete;
    BaseRwLock& operator=(BaseRwLock&) = delete;
    BaseRwLock& operator=(BaseRwLock&&) = delete;
    
    inline void readLock()
    { lock.lock_shared();}

    inline void readUnlock()
    { lock.unlock_shared();}

    inline void writeLock()
    { lock.lock();}

    inline void writeUnlock()
    { lock.unlock();}

private:
    std::shared_mutex lock;
};
