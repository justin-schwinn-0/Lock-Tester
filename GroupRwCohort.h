#pragma once

#include <atomic>
#include <functional>
#include <iostream>

struct CohortData
{
    std::atomic<bool> isCohortWaiting = false;
    // number of threads currently using the cohort
    std::atomic<uint32_t> cohortees = 0;
    std::atomic<bool> locked = true;

    static const uint32_t LAST_BIT = 0x80000000;
    static const uint32_t JOINABLE_MASK = 0x80000001;

    void setLocked(bool set)
    {
        if(set)
        {
            cohortees.fetch_or(LAST_BIT);
        }
        else
        {
            cohortees.fetch_and(~LAST_BIT);
        }
        locked.store(set);
    }

    void resetCohort()
    {
        cohortees.store(JOINABLE_MASK);
        locked=true;
        uint32_t tmp = 0;
    }

    bool giveUpSpot()
    {
        uint32_t lastVal = cohortees.fetch_sub(1);
        if(lastVal == 1)
        {
            isCohortWaiting.store(false);
            return true;
        }
        return false;
    }

    static bool canJoin(uint32_t val)
    {
        return val >= JOINABLE_MASK;
    }
};

template<class Lock, class RwLock, class CohortFunction,uint32_t N>
class GroupRwCohort
{
public:
    GroupRwCohort()
    {
        mRwLock.setCohortRelease(std::bind(&GroupRwCohort::giveUpCohort,this));
    }

    ~GroupRwCohort()
    {}

    GroupRwCohort(const GroupRwCohort&) = delete;
    GroupRwCohort(const GroupRwCohort&&) = delete;
    GroupRwCohort& operator=(GroupRwCohort&) = delete;
    GroupRwCohort& operator=(GroupRwCohort&&) = delete;

    // takes cohortId
    void getCohort()
    {
        auto& myCohort = getCohortData();

        // outer loop tries to get the global lock and make other threads wait for it
        while(true)
        {
            bool tmp = false;
            if(myCohort.isCohortWaiting.compare_exchange_strong(tmp,true))
            {
               myCohort.resetCohort(); 
               mLock.lock();
               myCohort.setLocked(false);
               return;
            }

            // inner loop joins cohorts, checks if it needs to rever to outer loop occasionally
            uint32_t spins = 0;
            while( spins < SPINS_MAX)
            {
                spins++;
                uint32_t expected = myCohort.cohortees.load();
                if(CohortData::canJoin(expected))
                {
                    if(myCohort.cohortees.compare_exchange_strong(expected,expected+1))
                    {
                        //Joined!
                        while(myCohort.locked){}
                        return;
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }

    void giveUpCohort()
    {
        auto& cohort = getCohortData();
        if(cohort.giveUpSpot())
        {
           mLock.unlock();
        }
    }

    void readLock()
    {
        getCohort();
        mRwLock.readLock();
    }

    void readUnlock()
    {
        mRwLock.readUnlock();
    }
    
    void writeLock()
    {
        getCohort();
        mRwLock.writeLock();
    }

    void writeUnlock()
    {
        mRwLock.writeUnlock();
    }

    CohortData& getCohortData()
    {
        static thread_local uint32_t cid = CohortFunction::cf();
        return cohorts[cid];
    }

private:
    Lock mLock;
    RwLock mRwLock;
    std::array<CohortData,N> cohorts;

    static const uint32_t SPINS_MAX = 10;
};
