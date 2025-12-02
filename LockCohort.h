#pragma once



template<typename NumaLock, typename RwLock>
class RwLockCohort
{
    LockCohort(int numaNodes)
    {
        cohorts = std::vector<CohortData>(numNodes); 
    }

    ~LockCohort(){}

    void lock()
    {
        CohortData& c = getCohort();

        c.count.fetch_add(1);
    }

    void unlock();

private:
    struct CohortData
    {
        std::atomic<int> count;
        RwLock rwLock;
    };
    std::vector<CohortData> cohorts;
    int mHandoffLimit;
    NumaLock mNumaLock;
};
