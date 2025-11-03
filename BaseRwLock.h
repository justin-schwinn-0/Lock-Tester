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
    
    void readLock()
    { lock.lock_shared();}

    void readUnlock()
    { lock.unlock_shared();}

    void writeLock()
    { lock.lock();}

    void writeUnlock()
    { lock.unlock();}

private:
    std::shared_mutex lock;
};
