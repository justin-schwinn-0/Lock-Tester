#ifndef TOURNAMENT_LOCK_H
#define TOURNAMENT_LOCK_H

#include <vector>
#include <cstdint>
#include <atomic>

#include "PetersonLock.h"

class TournamentLock 
{
public:
    explicit TournamentLock(int size);
    ~TournamentLock();

    TournamentLock(const TournamentLock&) = delete;
    TournamentLock(const TournamentLock&&) = delete;
    TournamentLock& operator=(TournamentLock&) = delete;
    TournamentLock& operator=(TournamentLock&&) = delete;
    
    void aquire(uint32_t me);
    void release(uint32_t me);
private:
    std::vector<PetersonLock> nodes;

};

#endif
