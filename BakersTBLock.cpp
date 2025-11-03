#include "BakersTBLock.h"

BakersTBLock::BakersTBLock(int size)
{
    flags = std::vector<Flag>(size);
    tokens = std::vector<PaddedAtomicInt>(size);
}

BakersTBLock::~BakersTBLock()
{}

void BakersTBLock::aquire(uint32_t me)
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

    for(int i =0; i < tokens.size(); i++)
    {
        while(flags[i] && tokenTest(i,me));
    }
}

bool BakersTBLock::tokenTest(int i, int me)
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

void BakersTBLock::release(uint32_t me)
{
    flags[me] = false;
}
