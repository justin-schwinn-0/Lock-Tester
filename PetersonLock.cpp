#include "PetersonLock.h"

PetersonLock::PetersonLock(int doesNothing)
{
    flags = std::vector<Flag>(2);
}

PetersonLock::~PetersonLock()
{}

void PetersonLock::aquire(uint32_t me)
{
    flags[me] = true;
    victim = me;

    while(victim == me && flags[flags.size()-me-1]);
}

void PetersonLock::release(uint32_t me)
{
    flags[me] = false;
}
