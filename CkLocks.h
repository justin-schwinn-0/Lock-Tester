#pragma once

#include "ck_tflock.h" 
#include "ck_rwlock.h" 
#include "ck_pflock.h" 

class CkTfLock
{
public:
    CkTfLock() :
        lock(CK_TFLOCK_TICKET_INITIALIZER)
    {
        ck_tflock_ticket_init(&lock);
    }
    ~CkTfLock(){}

    CkTfLock(const CkTfLock&) = delete;
    CkTfLock(const CkTfLock&&) = delete;
    CkTfLock& operator=(CkTfLock&) = delete;
    CkTfLock& operator=(CkTfLock&&) = delete;
    
    inline void readLock()
    {
        ck_tflock_ticket_read_lock(&lock);
    }

    inline void readUnlock()
    {
        ck_tflock_ticket_read_unlock(&lock);
    }

    inline void writeLock()
    {
        ck_tflock_ticket_write_lock(&lock);
    }

    inline void writeUnlock()
    {
        ck_tflock_ticket_write_unlock(&lock);
    }

private:
    ck_tflock_ticket lock;
};

class CkRwLock
{
public:
    CkRwLock() :
        lock(CK_RWLOCK_INITIALIZER)
    {
        ck_rwlock_init(&lock);
    }
    ~CkRwLock(){}

    CkRwLock(const CkRwLock&) = delete;
    CkRwLock(const CkRwLock&&) = delete;
    CkRwLock& operator=(CkRwLock&) = delete;
    CkRwLock& operator=(CkRwLock&&) = delete;
    
    inline void readLock()
    {
        ck_rwlock_read_lock(&lock);
    }

    inline void readUnlock()
    {
        ck_rwlock_read_unlock(&lock);
    }

    inline void writeLock()
    {
        ck_rwlock_write_lock(&lock);
    }

    inline void writeUnlock()
    {
        ck_rwlock_write_unlock(&lock);
    }

private:
    ck_rwlock lock;
};

class CkPfLock
{
public:
    CkPfLock() :
        lock(CK_PFLOCK_INITIALIZER)
    {
        ck_pflock_init(&lock);
    }
    ~CkPfLock(){}

    CkPfLock(const CkPfLock&) = delete;
    CkPfLock(const CkPfLock&&) = delete;
    CkPfLock& operator=(CkPfLock&) = delete;
    CkPfLock& operator=(CkPfLock&&) = delete;
    
    inline void readLock()
    {
        ck_pflock_read_lock(&lock);
    }

    inline void readUnlock()
    {
        ck_pflock_read_unlock(&lock);
    }

    inline void writeLock()
    {
        ck_pflock_write_lock(&lock);
    }

    inline void writeUnlock()
    {
        ck_pflock_write_unlock(&lock);
    }

private:
    ck_pflock lock;
};
