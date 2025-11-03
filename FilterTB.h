#ifndef FILTER_TB_LOCK_H
#define FILTER_TB_LOCK_H

#include <vector>
#include <cstdint>
#include <atomic>

#include "Flag.h"

class FilterTB 
{
public:
    explicit FilterTB(int size);
    ~FilterTB();

    FilterTB(const FilterTB&) = delete;
    FilterTB(const FilterTB&&) = delete;
    FilterTB& operator=(FilterTB&) = delete;
    FilterTB& operator=(FilterTB&&) = delete;
    
    void aquire(uint32_t me);
    void release(uint32_t me);
private:
    std::vector<PaddedAtomicInt> levels;
    std::vector<PaddedAtomicInt> victim;

};

#endif
