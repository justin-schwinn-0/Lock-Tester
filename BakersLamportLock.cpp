#include "BakersLamportLock.h"

BakersLamportLock::BakersLamportLock(int size)
{
    flags = std::vector<Flag>(size);
    tokens = std::vector<PaddedAtomicInt>(size);
}

BakersLamportLock::~BakersLamportLock()
{}

void BakersLamportLock::aquire(uint32_t me)
{
    flags[me] = true;
    int maxToken = 0;

    for(auto& paddedInt : tokens)
    {
        if(maxToken < paddedInt.get())
        {
            maxToken = paddedInt.get();
        }
    }
    tokens[me] = maxToken + 1;

    flags[me] = false;

    for(int i =0; i < tokens.size(); i++)
    {
        while(flags[i]);
        while(tokens[i].get() != 0 && tokenTest(i,me));
    }
    int enterCs =0;
}

bool BakersLamportLock::tokenTest(int i, int me)
{
    if(tokens[i].get() < tokens[me].get())
    {
        return true;
    }
    else if(tokens[i].get() == tokens[me].get())
    {
        if(i < me)
        {
            return true;
        }
    }

    return false;
}

void BakersLamportLock::release(uint32_t me)
{
    tokens[me] = 0;
}
