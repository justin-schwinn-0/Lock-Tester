#include "CnaLock.h"

#include <thread>

static thread_local cna_qnode mine; 

CnaLock::CnaLock(int size) :
    mTail(nullptr)
{
}

CnaLock::~CnaLock()
{
}

void CnaLock::aquire(uint32_t me)
{
    mine.next.store(nullptr);
    mine.socket = -1;
    mine.spin = 0;

    cna_qnode* pred = mTail.exchange(&mine);

    if(!pred)
    {
        mine.spin = 1;
        return;
    }

    mine.socket = currentNumaNode();

    pred->next.store(&mine);

    while(!mine.spin)
    {
        std::this_thread::yield();
    }
}

void CnaLock::release(uint32_t me)
{
    cna_qnode* next = mine.next.load();
    if(!next)
    {
        if(mine.spin == 1)
        {
            cna_qnode* tmp = &mine;
            if(mTail.compare_exchange_strong(tmp,static_cast<cna_qnode*>(nullptr)))
            {
                return;
            }
        }
        else
        {
            cna_qnode* sec = reinterpret_cast<cna_qnode*>(mine.spin);

            cna_qnode* tmp = &mine;
            if(mTail.compare_exchange_strong(tmp,sec->secTail))
            {
                sec->spin =1;
            }
        }

        while(mine.next.load() == nullptr)
        {
            std::this_thread::yield();
        }
    }

    cna_qnode* successor;

    if(keepLocal() && (successor = findSuccessor()))
    {
        successor->spin = mine.spin;
    }
    else if(mine.spin > 1)
    {
        successor = reinterpret_cast<cna_qnode*>(mine.spin);
        successor->secTail.load()->next.store(mine.next.load());
        successor->spin = 1;
    }
    else
    {
        cna_qnode* next = mine.next.load();
        next->spin = 1;
    }
}

int CnaLock::currentNumaNode()
{
    //TODO
    return 1;
}

int CnaLock::keepLocal()
{
    // TODO
    return 1;
}

cna_qnode* CnaLock::findSuccessor()
{
    cna_qnode* next = mine.next.load();
    int mySocket = mine.socket;
    if(mySocket == -1)
    {
        mySocket = currentNumaNode();
    }

    if(next->socket == mySocket)
    {
        // if next socket is the same as mine, mine->next can be next
        return next;
    }


    cna_qnode* secHead = next;
    cna_qnode* secTail = next;
    cna_qnode* cur = next->next;

    while(cur)
    {
        if(cur->socket == mySocket)
        {
            cna_qnode* otherHead = reinterpret_cast<cna_qnode*>(mine.spin);
            if(mine.spin > 1)
            {
                otherHead->secTail.load()->next.store(secHead);
            }
            else
            {
                mine.spin = reinterpret_cast<uintptr_t>(secHead);
            }

            secTail->next.store(nullptr);
            otherHead->secTail.store(secTail);
        }
        
        secTail = cur;
        cur = cur->next;
    }
    return nullptr;

}
