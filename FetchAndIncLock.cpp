#include "FetchAndIncLock.h"
#include "Utils.h"

#include <thread>
FetchAndIncLock::FetchAndIncLock(int size)
{
    turn = 0;
    token =0;
}
FetchAndIncLock::FetchAndIncLock()
{
    turn = 0;
    token =0;
}

FetchAndIncLock::~FetchAndIncLock()
{}

void FetchAndIncLock::lock()
{
    int myToken = token.fetch_add(1);

    while(turn != myToken)
    {
        //std::this_thread::yield();
    }
}

void FetchAndIncLock::unlock()
{
    turn++;
}
