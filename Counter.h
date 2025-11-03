#pragma once

struct Counter
{

    typedef std::pair<uint32_t,uint32_t> pair;

    Counter() : val(0) 
    {
    }
    ~Counter(){}

    pair getPair()
    {
        uint64_t v = val.load();
        uint32_t w,r;

        r = static_cast<uint32_t>(v);

        w = static_cast<uint32_t>(v >> 32);

        return {w,r};
    }

    uint64_t getUint64(const pair& v)
    {
        return static_cast<uint64_t>(v.first) << 32 | v.second;
    }

    pair FAI(uint32_t w,uint32_t r)
    {
        uint64_t newVal = static_cast<uint64_t>(w) << 32 | r;

        uint64_t old = val.fetch_add(newVal);

        uint32_t oldW,oldR;

        oldR = static_cast<uint32_t>(old);
        
        oldW = static_cast<uint32_t>(old >> 32);

        return {oldW,oldR};
    }

    pair FAD(uint32_t w,uint32_t r)
    {
        uint64_t newVal = static_cast<uint64_t>(w) << 32 | r;

        uint64_t old = val.fetch_sub(newVal);

        uint32_t oldW,oldR;

        oldR = static_cast<uint32_t>(old);
        
        oldW = static_cast<uint32_t>(old >> 32);

        return {oldW,oldR};
    }

    bool CAS(pair expected, pair newVal)
    {
        uint64_t exp = getUint64(expected);
        return val.compare_exchange_strong(exp,getUint64(newVal));
    }

    void setFirst(uint32_t a)
    {
        uint64_t prev = val.load();

        uint64_t newVal = static_cast<uint64_t>(a) << 32 | static_cast<uint32_t>(prev);
        val.store(newVal);
    }

    void setSecond(uint32_t b)
    {
        uint64_t prev = val.load();
        uint64_t newVal =  (prev & 0x00000000) | b;
        val.store(newVal);
    }

    void setBoth(uint32_t a, uint32_t b)
    {
        uint64_t newVal = static_cast<uint64_t>(a) << 32 | b;

        val.store(newVal);
    }

    uint32_t getFirst()
    {
        uint64_t v = val.load();

        return static_cast<uint32_t>(v >> 32);
    }

    uint32_t getSecond()
    {
        uint64_t v = val.load();

        return static_cast<uint32_t>(v);
    }

private:
    std::atomic<uint64_t> val;
};
