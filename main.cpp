#include <thread>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <numeric>
#include <functional>
#include <unistd.h>
#include <ctime>
#include <random>

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
#include "MrwLockOpt.h"
#include "McsRwNpGpt.h"
#include "McsRwRpGrok.h"

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
/*
template<class rwlock>
void rwThread
(
    rwlock& lock,
    bool& startBarrier,
    bool& continueFlag,
    uint64_t& w_iterations,
    uint64_t& r_iterations,
    std::function<void()> writeSection,
    std::function<void()> readSection,
    float writeRatio
)
{
    while(!startBarrier);

    while(continueFlag)
    {
        int rem = (w_iterations + r_iterations) % 10;
        if(rem == 0)
        {
            //write
            lock.writeLock();
            writeSection();
            lock.writeUnlock();
            w_iterations++;
        }
        else
        {
            //read
            lock.readLock();
            readSection();
            lock.readUnlock();
            r_iterations++;
        }
    }
}

template<class rwlock>
void rwRandThread
(
    rwlock& lock,
    bool& startBarrier,
    bool& continueFlag,
    uint64_t& w_iterations,
    uint64_t& r_iterations,
    std::function<void()> writeSection,
    std::function<void()> readSection,
    float writeRatio
)
{
    std::default_random_engine engine(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_real_distribution<float> dist(0.0f,100.0f);
    while(!startBarrier);

    while(continueFlag)
    {
        float pick = dist(engine);
        if(pick < writeRatio)
        {
            //write
            lock.writeLock();
            writeSection();
            lock.writeUnlock();
            w_iterations++;
        }
        else
        {
            //read
            lock.readLock();
            readSection();
            lock.readUnlock();
            r_iterations++;
        }
    }
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

std::vector<int32_t> dkWorkVec = std::vector<int32_t>(64);

template<class RwLockType>
void rwDiceKoganThread
(
    rwlock& lock,
    bool& startBarrier,
    bool& continueFlag,
    uint64_t& w_iterations,
    uint64_t& r_iterations,
    std::function<void()> writeSection,
    std::function<void()> readSection,
    float writeRatio
)
{
    std::default_random_engine engine(static_cast<unsigned int>(std::time(nullptr)));
    std::uniform_real_distribution<float> dist(0.0f,100.0f);
    while(!startBarrier);

    while(continueFlag)
    {
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
*/

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
std::pair<uint64_t, uint64_t> dynamicLockTestRandDist 
(
    const uint64_t numThreads,
    const int seconds,
    std::function<void()> w_section,
    std::function<void()> r_section,
    float ratio
)
{
    using namespace std::chrono_literals;
    std::vector<std::thread> threads;

    std::vector<uint64_t> readItrs = std::vector<uint64_t>(numThreads,0);
    std::vector<uint64_t> writeItrs = std::vector<uint64_t>(numThreads,0);

    bool start = false;
    bool continueFlag = true;

    RwLockType lock;

    for(int i = 0; i < numThreads; i++)
    {
        threads.push_back(std::thread(
                            rwRandThread<RwLockType>,
                            std::ref(lock),
                            std::ref(start),
                            std::ref(continueFlag),
                            std::ref(writeItrs[i]),
                            std::ref(readItrs[i]),
                            w_section,
                            r_section,
                            ratio));
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

    uint64_t totalReads = std::accumulate(readItrs.begin(), readItrs.end(),0);
    uint64_t totalWrites = std::accumulate(writeItrs.begin(), writeItrs.end(),0);

    return {totalReads,totalWrites};
}

/*
template<typename RwLockType>
void rwThrptTestRandDist
(
    const int seconds,
    const std::vector<int> threadCounts,
    const std::string& name,
    std::function<void()> w_section,
    std::function<void()> r_section,
    float ratio
)
{
    for(int tcount : threadCounts)
    {
        auto [rs,ws] = dynamicLockTestRandDist<RwLockType>(
                            tcount,
                            seconds,
                            w_section,
                            r_section,
                            ratio); 
        double rRate = rs / static_cast<double>(seconds);
        double wRate = ws / static_cast<double>(seconds);
        printf("%s,%d,%.2e,%.2e\n",name.c_str(),tcount,rRate,wRate);
    }
}

template<typename RwLockType>
void rwThrptTests
(
    const int seconds,
    const std::vector<int> threadCounts,
    const std::string& name,
    const ThreadFn<RwLockType> fn
)
{
    for(int tcount : threadCounts)
    {
        printf("%s,%d,",name.c_str(),tcount);
        uint64_t itrs = dynamicLockTest<RwLockType>(tcount,seconds,fn); 
        double itrRate = itrs / static_cast<double>(seconds);
        printf("%.2e\n",itrRate);
    }
}
*/

// run a 90-10 R-W test for [seconds] number of seconds
// run a test for each thread count in [threadCounts]
// name is name of test
template<class RwLockType>
void runRwCorrectnessTestsForLock
(
    const uint64_t seconds, 
    const std::vector<int> threadCounts,
    const std::string name,
    const bool verbose
)
{
    printf("Running \"Correctness\" test for %s\n",name.c_str());
    uint64_t totalWriteFails = 0;
    uint64_t totalReadFails = 0;
    uint64_t totalIterations = 0;

    for(int tCount : threadCounts)
    {
        auto [wFails,rFails,itrs] = runRwTest<RwLockType>(tCount,seconds); 

        totalWriteFails += wFails;
        totalReadFails += rFails;
        totalIterations += itrs;

        if(verbose)
        {
            double rate = itrs/ static_cast<double>(seconds);
            printf("T: %d   -> %.3e /s, %ld write fails, %ld read fails\n",
                    tCount,
                    rate,
                    wFails,
                    rFails);
        }
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
    std::string distType = "";
    std::string csType = "";
    float writeRatio = 10.0;
    int stride = 8;
    int strideThreshold = 32;
    bool verbose = false;

};

void rwThrptTest
(
    const TestOptions& opt,
    const std::vector<int> threadCounts,
    std::function<bool()> distribution
    std::function<void()> w_section,
    std::function<void()> r_section,
)
{

}


template<typename lock>
void doCsTest(const TestOptions& opt)
{
    int threads = std::thread::hardware_concurrency();

    if(opt.csType == "correctness")
    {
        runRwCorrectnessTestsForLock<lock>(
            opt.time,
            getCompConfig(threads,opt.stride,opt.strideThreshold),
            opt.lockType,
            opt.verbose);
    }

    else if(opt.csType == "empty-cs")
    {
        rwThrptTests<lock>(
            opt.time,
            getCompConfig(threads,opt.stride,opt.strideThreshold),
            opt.name +","+opt.csType,
            &rwEmptyThread<lock>);
    }
    else if(opt.csType == "small-cs")
    {
        rwThrptTests<lock>(
            opt.time,
            getCompConfig(threads,opt.stride,opt.strideThreshold),
            opt.name +","+opt.csType,
            &rwSmallWork<lock>);
    }
    else if(opt.csType == "empty-cs-2")
    {
        rwThrptTest2<lock>(
            opt.time,
            getCompConfig(threads,opt.stride,opt.strideThreshold),
            opt.name +","+opt.csType,
            [](){},
            [](){},
            opt.writeRatio);
    }
    else
    {
        printf("Unknown test Type! %s",opt.csType.c_str());
    }
}

void doDistribution
(
    const TestOptions& opt
)
{
    if(opt.distType == "static")
    {
        
    }
    else if(opt.distType == "random")
    {

    }
    else 
    {
        printf("Unknown Distribution function: %s", opt.distType.c_str());
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
        printf("Unknown lock type: %s",opt.lockType.c_str());
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

    parser.addOption("--ratio",
            [&](const std::string& s)
            {
                test.writeRatio = Utils::strToFloat(s);
            },true);

    parser.addOption("-v",
            [&](const std::string& s)
            {
                test.verbose = true;
            },false);

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
