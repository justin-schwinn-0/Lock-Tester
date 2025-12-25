#include <thread>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <numeric>
#include <functional>
#include <unistd.h>

//mutex locks
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

//rw locks
#include "BaseRwLock.h"
#include "CrmrRwLock.h"
#include "MrwLock.h"
#include "MrwLockOpt.h"

#include "OptionParser.h"

template<typename Lock>
using ThreadFn = void(*)(Lock&,bool&,bool&,uint64_t&);

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
void RwThreadCorrectness
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
            if(!writeTest(writtenBlock,tid))
            {
                writeFails++;
            }
            lock.writeUnlock();
        }
        else
        {
            lock.readLock();
            if(!readTest(writtenBlock))
            {
                readFails++;
            }
            lock.readUnlock();
        }
    }
}

template<class rwlock>
void rwCsThread
(
    rwlock& lock,
    bool& startBarrier,
    bool& continueFlag,
    std::function<void()> writeSection,
    std::function<void()> readSection,
    uint64_t& w_iterations,
    uint64_t& r_iterations
)
{

}

template<class RwLockType>
void rwEmptyThread
(
    RwLockType& lock,
    bool& startBarrier,
    bool& continueFlag,
    uint64_t& iterations
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
            lock.writeUnlock();
        }
        else
        {
            lock.readLock();
            lock.readUnlock();
        }
    }
}

std::vector<uint64_t> smallWorkVec = std::vector<uint64_t>(16,16);

template<class RwLockType>
void rwSmallWork 
(
    RwLockType& lock,
    bool& startBarrier,
    bool& continueFlag,
    uint64_t& iterations
)
{
    pid_t pid = getpid();
    while(!startBarrier);

    while(continueFlag)
    {
        int rem = iterations % 10;
        iterations++;
        if(rem == 0)
        {
            lock.writeLock();
            writeTest(smallWorkVec,pid);
            lock.writeUnlock();
        }
        else
        {
            lock.readLock();
            readTest(smallWorkVec);
            lock.readUnlock();
        }
    }
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

    std::vector<uint64_t> writeFails = std::vector<uint64_t>(numThreads,0);
    std::vector<uint64_t> readFails = std::vector<uint64_t>(numThreads,0);
    std::vector<uint64_t> iterations = std::vector<uint64_t>(numThreads,0);

    bool start = false;
    bool continueFlag = true;

    RwLockType lock;

    for(int i = 0; i < numThreads; i++)
    {
        threads.push_back(std::thread(
                            &RwThreadCorrectness<RwLockType>,
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

template<typename RwLockType>
uint64_t dynamicLockTest 
(
    const uint64_t numThreads,
    const int seconds,
    const ThreadFn<RwLockType> fn
)
{
    using namespace std::chrono_literals;
    std::vector<std::thread> threads;

    std::vector<uint64_t> iterations = std::vector<uint64_t>(numThreads,0);

    bool start = false;
    bool continueFlag = true;

    RwLockType lock;

    for(int i = 0; i < numThreads; i++)
    {
        threads.push_back(std::thread(
                            fn,
                            std::ref(lock),
                            std::ref(start),
                            std::ref(continueFlag),
                            std::ref(iterations[i])));
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

    uint64_t totalIterations = std::accumulate(iterations.begin(), iterations.end(),0);

    return totalIterations;
}

template<typename RwLockType>
void rwThrptTests
(
    const int seconds,
    const std::vector<int> threadCounts,
    const int trials,
    const std::string& name,
    const ThreadFn<RwLockType> fn
)
{
    for(int tcount : threadCounts)
    {
        printf("%s,%d,",name.c_str(),tcount);
        for(int i = 0 ; i < trials; i++)
        {
            uint64_t itrs = dynamicLockTest<RwLockType>(tcount,seconds,fn); 
            double itrRate = itrs / static_cast<double>(seconds);
            printf("%.2e",itrRate);
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

std::vector<int> getTConfig(int iter, int thread)
{
    return std::vector<int>(iter,thread);
}

// returns a comprehensive config for tests
std::vector<int> getCompConfig(int maxT, int stride = 8,int threshold = 32)
{
    if(maxT < threshold)
    {
        std::vector<int> vec;
        for(int i = 1;i <= maxT ; i++)
        {
            vec.push_back(i);
        }

        return vec;
    }
    else
    {
        std::vector<int> vec = {1};

        for(int i = stride;i <= maxT ; i+=stride)
        {
            vec.push_back(i);
        }

        return vec;
    }
}


struct TestOptions
{
    int time = -1;
    std::string name = "";
    std::string lockType = "";
    std::string csType = "";
    float writeRatio = 0.1;
    int stride = 8;
    int strideThreshold = 32;

};

template<typename lock>
void doCsTest(const TestOptions& opt)
{
    int threads = std::thread::hardware_concurrency();

    if(opt.csType == "empty-cs")
    {
        rwThrptTests<lock>(
            opt.time,
            getCompConfig(threads,opt.stride,opt.strideThreshold),
            1,
            opt.name +","+opt.csType,
            &rwEmptyThread<lock>);
    }
    else if(opt.csType == "small-cs")
    {
        rwThrptTests<lock>(
            opt.time,
            getCompConfig(threads,opt.stride,opt.strideThreshold),
            1,
            opt.name +","+opt.csType,
            &rwSmallWork<lock>);
    }
}

void runTest
(
    const TestOptions& opt
)
{
    if(opt.lockType == "mrw-opt")
    {
        doCsTest<MrwLockOpt>(opt);
    }
    else if(opt.lockType == "mrw")
    {
        doCsTest<MrwLock>(opt);
    }
    else if(opt.lockType == "crmr-w")
    {
        doCsTest<CrmrRwLock>(opt);
    }
    else if(opt.lockType == "cpp-std")
    {
        doCsTest<BaseRwLock>(opt);
    }
    else
    {
        printf("Unknown lock type:",opt.lockType.c_str());
    }
}

int main(int argc, char** argv)
{   
    OptionParser parser;
    TestOptions test;

    parser.addOption("--time",
            [&](const std::string& s)
            {
                test.time = Utils::strToInt(s);
            },true);

    parser.addOption("--name",
            [&](const std::string& s)
            {
                test.name = s;
            },true);

    parser.addOption("--lockType",
            [&](const std::string& s)
            {
                test.lockType = s;
            },true);

    parser.addOption("--csType",
            [&](const std::string& s)
            {
                test.csType = s;
            },true);

    parser.addOption("--stride",
            [&](const std::string& s)
            {
                test.stride = Utils::strToInt(s);
            },true);

    parser.addOption("--threshold",
            [&](const std::string& s)
            {
                test.strideThreshold = Utils::strToInt(s);
            },true);

    parser.parse(argc,argv);

    if(test.name.empty())
    {
        printf("Test Must Be named: %s\n",test.name.c_str());
        std::exit(1);
    }
    else if(test.lockType.empty())
    {
        printf("Test Must specify lock\n");
        std::exit(1);
    }
    else if(test.csType.empty())
    {
        printf("Test Must specify critical section\n");
        std::exit(1);
    }
    else if(test.time <= 0)
    {
        printf("Test must have positive time\n");
        std::exit(1);
    }
    
    runTest(test);
}
