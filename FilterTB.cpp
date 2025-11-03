#include "FilterTB.h"
#include "Utils.h"

FilterTB::FilterTB(int size)
{
    levels = std::vector<PaddedAtomicInt>(size);
    victim = std::vector<PaddedAtomicInt>(size-1);
}

FilterTB::~FilterTB()
{}

void FilterTB::aquire(uint32_t me)
{
    int i;
    int lvl;
    bool keepChecking = true;
    for(lvl = 1; lvl < levels.size(); lvl++)
    {
        levels[me] = lvl;
        victim[lvl-1] = me;
        keepChecking = true;
        while(victim[lvl-1].get() == me && keepChecking)
        {
            keepChecking = false;
            for(i = 0; i < levels.size() ; i++)
            {
                if(i != me && levels[i].get() >= lvl)
                {
                    keepChecking = true;
                }
            }
        }
    }

    int exitTest = 1;

}

void FilterTB::release(uint32_t me)
{
    int release = 1;
    levels[me] = 0;
}
