#ifndef GPL_LOCK_H
#define GPL_LOCK_H

#include <vector>
#include <cstdint>
#include <atomic>

#include "Flag.h"

class Gpl 
{
public:
    explicit Gpl(int size);
    ~Gpl();

    /*Gpl(const Gpl&) = delete;
    Gpl(const Gpl&&) = delete;
    Gpl& operator=(Gpl&) = delete;
    Gpl& operator=(Gpl&&) = delete;*/

    void aquire(uint32_t me);
    void release(uint32_t me);


private:
    std::vector<Flag> flags;
    std::atomic<int> victim;
};

#endif
