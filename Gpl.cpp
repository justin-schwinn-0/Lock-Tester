#include "Gpl.h"

Gpl::Gpl(int size)
{
    flags = std::vector<Flag>(size);
}

Gpl::~Gpl()
{}

void Gpl::aquire(uint32_t me)
{
    flags[me] = true;
    victim = me;
    bool checkFlags = true;

    while(victim == me && checkFlags)
    {
        checkFlags=false;
        for(int i = 0; i < flags.size() ; i++)
        {
            if(i != me && flags[i])
            {
                checkFlags=true;
            }
        }
    }
}

void Gpl::release(uint32_t me)
{
    flags[me] = false;
}

