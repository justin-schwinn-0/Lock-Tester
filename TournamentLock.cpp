#include "TournamentLock.h"
#include <cmath>
#include "Utils.h"

TournamentLock::TournamentLock(int size)
{
    int treeSize = std::pow(2,std::ceil(std::log2(size)))-1;
    nodes = std::vector<PetersonLock>(treeSize); //assume power of 2
}

TournamentLock::~TournamentLock()
{}

void TournamentLock::aquire(uint32_t me)
{
    int total = nodes.size()+1;
    int offset = 0;
    int localMe = me;

    while(total != 1)
    {
        int instance = localMe /2;
        int side = localMe % 2;
        nodes[instance+offset].aquire(side);
        localMe = instance;
        total = total/2;
        offset = offset + total;
    }
    int enterCS = 1;
}

void TournamentLock::release(uint32_t me)
{
    int total = nodes.size()+1;
    int offset = 0;
    int localMe = me;

    std::vector<std::pair<int,int>> releaseOrder;

    while(total != 1)
    {
        int instance = localMe /2;
        int side = localMe % 2;
        releaseOrder.push_back(std::pair<int,int>(instance+offset,side));
        localMe = instance;
        total = total/2;
        offset = offset + total;
    }

    int startRelease = 1;
    for(int i = releaseOrder.size()-1; i >= 0; i--)
    {
        auto& pair= releaseOrder[i];
        nodes[pair.first].release(pair.second);
    }
}
