#ifndef FILTER_BB_LOCK_H
#define FILTER_BB_LOCK_H

#include <vector>
#include <cstdint>

#include "Gpl.h"
#include <memory>

class FilterBB 
{
public:
    explicit FilterBB(int size);
    ~FilterBB();

    FilterBB(const FilterBB&) = delete;
    FilterBB(const FilterBB&&) = delete;
    FilterBB& operator=(FilterBB&) = delete;
    FilterBB& operator=(FilterBB&&) = delete;
    
    void aquire(uint32_t me);
    void release(uint32_t me);
private:
    std::vector<std::shared_ptr<Gpl>> levels;
};

#endif
