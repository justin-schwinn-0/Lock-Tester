#include <thread>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <numeric>

#include "Utils.h"
#include "PetersonLock.h"
#include "Gpl.h"
#include "FilterBB.h"
#include "FilterTB.h"
#include "TournamentLock.h"
#include "BakersLamportLock.h"
#include "BakersTBLock.h"
#include "FetchAndIncLock.h"
#include "McsLock.h"
#include "CnaLock.h"
#include "BaseRwLock.h"
#include "CrmrRwLock.h"
#include "MrwLock.h"

// our critial section
template<class T>
void incrementValueForThread
(
    uint64_t& val, 
    const int goalNum,
    const int tid,
    const int iterations,
    std::vector<uint64_t>& turnAroundTimesVec,
    T& lock)
{

    for(int i = 0; i < iterations; i++)
    {
        int turnAroundIndex;
        auto start = std::chrono::high_resolution_clock::now();
        lock.aquire(tid);
        turnAroundIndex = val;
        val = val + 1;
        lock.release(tid);
        auto end = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

        turnAroundTimesVec[turnAroundIndex] = dur;
    }
}

bool readTest(std::vector<uint64_t>& vec)
{
    uint64_t initVal = vec[0];
    uint64_t sum = std::accumulate(vec.begin(),vec.end(),0);

    return sum == initVal * vec.size();
}

bool writeTest(std::vector<uint64_t>& vec,uint64_t setVal)
{
    // if readTest fails, a write test is happening other than mine
    if(readTest(vec))
    {
        for(int i = 0; i < vec.size(); i++)
        {
           vec[i] = setVal;
        }

        uint64_t sum = std::accumulate(vec.begin(),vec.end(),0);
        
        // if i sum elements and they aren't mine, another write is happening
        if(sum != setVal * vec.size())
        {
            return false;
        }
    }
    else
    {
        return false;
    }

    // writes can't detect reads, but reads can detect writes
    return true;
}

template<class RwLockType>
void RwThread
(
    RwLockType& lock,
    std::vector<uint64_t>& writtenBlock,
    bool& startBarrier,
    bool& continueFlag,
    uint64_t& writeFails,
    uint64_t& readFails,
    uint64_t& iterations,
    uint64_t tid
)
{
    while(!startBarrier);

    while(continueFlag)
    {
        int rem = iterations % 10;
        iterations++;
        if(rem == 0)
        {
            lock.writeLock();
            /*
            //lock.print();
            if(!writeTest(writtenBlock,tid))
            {
                writeFails++;
            }
            */
            lock.writeUnlock();
        }
        else
        {
            lock.readLock();
            /*
            if(!readTest(writtenBlock))
            {
                readFails++;
            }
            */
            lock.readUnlock();
        }
    }
}

template<class LockType>
double runTest
(
    LockType& lock,
    const uint64_t goalNum, 
    const uint64_t numThreads,
    std::vector<uint64_t>& turnAroundTimesVec
)
{

    auto start = std::chrono::high_resolution_clock::now();
    std::vector<std::thread> threads;
    uint64_t val = 0;

    int iterationsPerThread = goalNum/numThreads;


    for(int i = 0; i < numThreads; i++)
    {
        threads.push_back(std::thread(&incrementValueForThread<LockType>,
                          std::ref(val),
                          goalNum,
                          i,
                          iterationsPerThread,
                          std::ref(turnAroundTimesVec),
                          std::ref(lock)));
    }

    for(int i = 0; i < numThreads; i++)
    {
        threads[i].join();
    }

    if(val != (iterationsPerThread * numThreads))
    {
        return -1;
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    
    double seconds = dur / 1e6;

    return seconds;
}

template<class RwLockType>
std::tuple<uint64_t,uint64_t,uint64_t> runRwTest
(
    const uint64_t numThreads,
    int seconds
)
{
    using namespace std::chrono_literals;
    std::vector<std::thread> threads;

    std::vector<uint64_t> targetBlock = std::vector<uint64_t>(16,16);

    std::vector<uint64_t> writeFails = std::vector<uint64_t>(16,0);
    std::vector<uint64_t> readFails = std::vector<uint64_t>(16,0);
    std::vector<uint64_t> iterations = std::vector<uint64_t>(16,0);

    bool start = false;
    bool continueFlag = true;

    RwLockType lock;

    for(int i = 0; i < numThreads; i++)
    {
        threads.push_back(std::thread(
                            &RwThread<RwLockType>,
                            std::ref(lock),
                            std::ref(targetBlock),
                            std::ref(start),
                            std::ref(continueFlag),
                            std::ref(writeFails[i]),
                            std::ref(readFails[i]),
                            std::ref(iterations[i]),
                            i));
    }

    std::this_thread::sleep_for(50ms);
    auto startTime = std::chrono::high_resolution_clock::now();

    start = true;

    while(continueFlag)
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        auto dur = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count();

        if(dur >= seconds)
        {
           continueFlag = false; 
        }

    }

    for(int i = 0; i < numThreads; i++)
    {
        threads[i].join();
    }

    uint64_t totalWriteFails = std::accumulate(writeFails.begin(), writeFails.end(),0);
    uint64_t totalReadFails = std::accumulate(readFails.begin(), readFails.end(),0);
    uint64_t totalIterations = std::accumulate(iterations.begin(), iterations.end(),0);

    auto tup = std::make_tuple(totalWriteFails,totalReadFails,totalIterations);

    return tup;
}

template<typename rwlock>

void rwTestEmpty
(
)
{

}

std::string strSpace(int num)
{
    std::ostringstream str;
    for(int i = 0; i < num ; i++)
    {
        str << " ";
    }

    return str.str();
}


template<typename rwlock>
void rwThrptTests
(
    const int seconds,
    const std::vector<int> threadCounts,
    const int trials,
    const std::string& name
)
{
    printf("Running Tests for %s\n",name.c_str());
    for(int tcount : threadCounts)
    {
        printf("T:%d",tcount);
        for(int i = 0 ; i < trials; i++)
        {
            auto [wFails,rFails,itrs] = runRwTest<rwlock>(tcount,seconds); 
            double itrRate = itrs / static_cast<double>(seconds);
            printf("\t%.2e",itrRate);
            fflush(stdout);
        }
        printf("\n");
    }
}

// run a 90-10 R-W test for [seconds] number of seconds
// run a test for each thread count in [threadCounts]
// name is name of test
template<class RwLockType>
void runRwCorrectnessTestsForLock
(
    const uint64_t seconds, 
    const std::vector<int> threadCounts,
    const std::string name
)
{
    printf("Running \"Correctness\" test for %s",name.c_str());
    uint64_t totalWriteFails = 0;
    uint64_t totalReadFails = 0;
    uint64_t totalIterations = 0;

    for(int tCount : threadCounts)
    {
        auto [wFails,rFails,itrs] = runRwTest<RwLockType>(tCount,seconds); 

        totalWriteFails += wFails;
        totalReadFails += rFails;
        totalIterations += itrs;
    }

    double avgWriteFails = static_cast<double>(totalWriteFails) / threadCounts.size();
    double avgReadFails = static_cast<double>(totalReadFails) / threadCounts.size();
    double avgIterations = static_cast<double>(totalIterations) / threadCounts.size();

    double iterRate = avgIterations / static_cast<double>(seconds);


    if(totalWriteFails > 0 || totalReadFails > 0)
    {
        printf("Test Fail Averages\n");
        printf("    Write-Fails: %.2e (%ld total)\n",avgWriteFails,totalWriteFails);
        printf("    Read-Fails:  %.2e (%ld total)\n",avgReadFails,totalReadFails);
        printf("    Iterations:  %.2e (%ld total) (%.3e /s)\n",avgIterations,totalIterations,iterRate);
    }
    else
    {
        printf("    %.3e /s Iterations\n",iterRate);
    }

}

template<class LockType>
void runTestsForLock
(
    const uint64_t goalNum, 
    const std::vector<int> threadCounts,
    const std::string name = ""
)
{
    const int columnWidth = 15;
    Utils::log("Running",name,"tests"); // blank line
    std::ostringstream timeLine;
    std::ostringstream turnLine;
    std::ostringstream rateLine;
    std::ostringstream threadLine;

    std::vector<uint64_t> turnAroundTimes =std::vector<uint64_t>(goalNum);

    std::cout <<  "Completed Tests:  ";
    timeLine  <<  "Times:            ";
    turnLine  <<  "Turn Around Time: ";
    rateLine  <<  "Rates:            ";

    for(int threadCount : threadCounts)
    {
        LockType lock(threadCount);
        double time = runTest(lock,goalNum,threadCount,turnAroundTimes);
        double rate = goalNum / time;

        std::string thrdStr = std::to_string(threadCount);

        std::cout << thrdStr << strSpace(columnWidth - thrdStr.length()) << std::flush;
        std::ostringstream currTimeLine;
        std::ostringstream currRateLine;
        std::ostringstream currTurnLine;
        currTimeLine << std::setprecision(4) << time << "s"; 
        currRateLine << std::scientific << std::setprecision(4) << rate  << "/s"; 

        uint64_t runningTotalTime = 0;
        for(uint64_t time : turnAroundTimes)
        {
            runningTotalTime += time;
        }

        uint64_t avgTurnTime = runningTotalTime / turnAroundTimes.size();
        currTurnLine  << avgTurnTime << "ns"; 

        std::string currTimeLineStr = currTimeLine.str();
 
        std::string currRateLineStr = currRateLine.str();
        std::string currTurnLineStr = currTurnLine.str();

        timeLine << currTimeLineStr << strSpace(columnWidth - currTimeLineStr.length());
        rateLine << currRateLineStr << strSpace(columnWidth - currRateLineStr.length());
        turnLine << currTurnLineStr << strSpace(columnWidth - currTurnLineStr.length());
    }

    std::cout << std::endl;

    Utils::log(timeLine.str());
    Utils::log(rateLine.str());
    Utils::log(turnLine.str());
    Utils::log("");
}

std::vector<int> getTConfig(int iter, int thread)
{
    return std::vector<int>(iter,thread);
}

// returns a comprehensive config for tests
std::vector<int> getCompConfig(int maxT)
{
    std::vector<int> vec;
    for(int i = 1;i <= maxT ; i++)
    {
        vec.push_back(i);
    }

    return vec;
}

int main()
{   
    const int numThreads = std::thread::hardware_concurrency();

    const uint64_t bigGoalNum = 1e7;
    const uint64_t littleGoalNum = 2e3;
    const uint64_t threadCount = 8;

    const int testIterations = 3;

    std::vector<int> threadsSmall = {2};
    std::vector<int> threadMid = {2,4,8};
    std::vector<int> threadLarge = {16,16,16,16,16};
    printf("Running tests on %d threads\n",numThreads);

/*
    runTestsForLock<PetersonLock>(bigGoalNum,threadsSmall,"Peterson Lock");

    runTestsForLock<Gpl>(bigGoalNum,threadsSmall,"2T Generalized Peterson Lock");

    runTestsForLock<FilterBB>(bigGoalNum,threadLarge,"Filter Black Box");

    runTestsForLock<FilterTB>(bigGoalNum,threadLarge,"Filter Textbook"); 

    runTestsForLock<TournamentLock>(bigGoalNum,threadLarge,"Tournament Tree Lock");

    runTestsForLock<BakersLamportLock>(bigGoalNum,threadLarge,"Lamport's Bakers Lock");

    runTestsForLock<BakersTBLock>(bigGoalNum,threadLarge,"TB Bakers Lock");

    runTestsForLock<FetchAndIncLock>(bigGoalNum,ts_4_2,"FAI Lock");
*/

    //runTestsForLock<McsLock>(littleGoalNum,getTConfig(1,8),"MCS Lock");

    //runTestsForLock<CnaLock>(bigGoalNum,ts_4_2,"CNA Lock");

    //runRwTestsForLock<BaseRwLock>(1,getTConfig(1,8),"C++ RW Lock");

    //runRwCorrectnessTestsForLock<CrmrRwLock>(10,getTConfig(1,8),"CRMR RW Lock");

    //runRwCorrectnessTestsForLock<MrwLock>(10,getTConfig(1,8),"MRW Lock");
    
    rwThrptTests<CrmrRwLock>(1,getCompConfig(numThreads),testIterations,"CRMR RW Lock");
    rwThrptTests<MrwLock>(1,getCompConfig(numThreads),testIterations, "MRW Lock");
}
