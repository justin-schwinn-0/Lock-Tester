#include "FilterBB.h"
#include "Utils.h"

FilterBB::FilterBB(int size)
{
    for(int i = 0; i < size; i++)
    {
        levels.emplace_back(std::make_shared<Gpl>(size));
    }
}

FilterBB::~FilterBB()
{}

void FilterBB::aquire(uint32_t me)
{
    for(int i = 1; i < levels.size(); i++)
    {
        levels[i]->aquire(me);
    }
    int test = me;
}

void FilterBB::release(uint32_t me)
{
    int test = me;
    for(int i = levels.size()-1; i >= 1; i--)
    {
        levels[i]->release(me);
    }
}
