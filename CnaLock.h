#ifndef CNA_LOCK_H 
#define CNA_LOCK_H

#include <vector>
#include <cstdint>
#include <atomic>

struct cna_qnode
{
    std::uintptr_t spin;
    int socket;
    std::atomic<cna_qnode*> next;
    std::atomic<cna_qnode*> secTail;
};

class CnaLock 
{
public:
    explicit CnaLock(int size);
    ~CnaLock();

    CnaLock(const CnaLock&) = delete;
    CnaLock(const CnaLock&&) = delete;
    CnaLock& operator=(CnaLock&) = delete;
    CnaLock& operator=(CnaLock&&) = delete;
    
    void aquire(uint32_t me);
    void release(uint32_t me);

    cna_qnode* findSuccessor();
    int keepLocal();

    int currentNumaNode();
private:
    std::atomic<cna_qnode*> mTail;
};


#endif
