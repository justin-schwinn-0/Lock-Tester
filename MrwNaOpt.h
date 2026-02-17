#pragma once

#include <vector>
#include <cstdint>
#include <atomic>
#include <functional>


template<int MASK_SIZE>
struct MaskedMirrorCondCntr
{
    // MASK_SIZE bits aren't used in the number we use 
    alignas(64) std::atomic<uint32_t> maskedCount;

    // contains a mirror of the first MASK_SIZE bits
    // so that we can read the bits without 
    // incurring RMR for each count update
    alignas(64) std::atomic<uint32_t> mirror;

    // this is basicly a group mutex the more i look at it


    // if mirror is empty or your mask check passes
    bool check(uint32_t mask)
    {
       return mirror == 0 || (mirror & mask == mirror); 
    }

    uint32_t getCount()
    {
        return maskedCount.load() >> MASK_SIZE;
    }

    bool tryInc(uint32_t myMask)
    {
        uint32_t masked = maskedCount.load();
        uint32_t count = masked >> MASK_SIZE;
        uint32_t newCount = ((count+1) << MASK_SIZE) | myMask;

        if(masked == 0)
        {
            if(maskedCount.compare_exchange_strong(masked,newCount))
            {
                mirror = myMask;
                return true;
            }
            else
            {
                return false;
            }
        }

        return maskedCount.compare_exchange_strong(masked,newCount);
    }

    void leave()
    {
        uint32_t lastVal = maskedCount.fetch_sub(1 << MASK_SIZE);

        if((lastVal >> MASK_SIZE) == 1)
        {
            maskedCount.store(0);
            mirror.store(0);
        }
        
    }
};

struct mrwnao_qnode
{
    // cap of 2^31, bit 31 is for locked
    alignas(64) std::atomic<uint32_t> count;
    alignas(64) bool locked;

    MaskedMirrorCondCntr<5> numaCounter = {0,0};
    std::atomic<mrwnao_qnode*> next;
};

class MrwNaOptLock 
{
public:
    explicit MrwNaOptLock();
    ~MrwNaOptLock();

    MrwNaOptLock(const MrwNaOptLock&) = delete;
    MrwNaOptLock(const MrwNaOptLock&&) = delete;
    MrwNaOptLock& operator=(MrwNaOptLock&) = delete;
    MrwNaOptLock& operator=(MrwNaOptLock&&) = delete;
    
    void readLock();
    void readUnlock();
    void writeLock();
    void writeUnlock();

    void performAquire(mrwnao_qnode* node);
    void performRelease(mrwnao_qnode* node);

    bool isLocked(uint32_t counter);
    void setLocked(mrwnao_qnode* node, bool set);
    void resetNode(mrwnao_qnode* node);

    inline bool spin(mrwnao_qnode* node)
    {

        return node->locked;
        //return isLocked(node->count.load());
    }

    void print();
    uint32_t getGroup();

    static void setGroupFunction(std::function<uint32_t()> pFunc)
    { sGroupFunction = pFunc; }


private:
    std::atomic<mrwnao_qnode*> mTail;

    static inline std::function<uint32_t()> sGroupFunction;

    static constexpr uint32_t LOCKED_READING_START_MASK = 0x80000001;
    static constexpr uint32_t LAST_BIT_MASK = 1 << 31; 
};
