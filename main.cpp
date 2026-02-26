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
#include <cmath>
#include <sched.h>

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
#include "CrmrRwLockR.h"
#include "MrwNaOpt.h"
#include "MrwLockSc.h"

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
        for(int i = vec.size()-1; i >= 0; i--)
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
    std::string groupFunc = "";
    float writeRatio = 10.0;
    int stride = 8;
    int strideThreshold = 32;
    bool verbose = false;
    std::function<bool()> distribution;
    std::function<void()> w_section;
    std::function<void()> r_section;
};

struct Aligned64bUint
{
    alignas(64) uint64_t value;
};

template<class RwLock>
void rwThrptTest
(
    const TestOptions& opt
)
{
    using namespace std::chrono_literals;
    int maxThreads = std::thread::hardware_concurrency();
    auto config = getCompConfig(maxThreads,opt.stride,opt.strideThreshold);

    auto dist = opt.distribution;
    auto w_section = opt.w_section;
    auto r_section = opt.r_section;

    for(int numThreads : config)
    {

        RwLock lock;

        bool startBarrier = false;

        bool continueFlag = true;

        auto thread = [&](Aligned64bUint& wItrs, Aligned64bUint& rItrs)
        {
            while(!startBarrier);

            while(continueFlag)
            {
                if(dist())
                {
                    //write
                    lock.writeLock();
                    w_section();
                    lock.writeUnlock();
                    wItrs.value++;
                }
                else
                {
                    // read
                    lock.readLock();
                    r_section();
                    lock.readUnlock();
                    rItrs.value++;
                }
            }
        };

        std::vector<std::thread> threads;
        std::vector<Aligned64bUint> writeIterations = std::vector<Aligned64bUint>(numThreads,{0});
        std::vector<Aligned64bUint> readIterations =  std::vector<Aligned64bUint>(numThreads,{0});

        for(int i = 0; i < numThreads;i++)
        {
            threads.push_back(std::thread(
                    thread,
                    std::ref(writeIterations[i]),
                    std::ref(readIterations[i])));
        }

        std::this_thread::sleep_for(50ms);
        auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();

        startBarrier = true;

        std::this_thread::sleep_for(std::chrono::seconds(opt.time));

        continueFlag = false; 
        for(int i = 0; i < numThreads; i++)
        {
            threads[i].join();
        }

        uint64_t totalWrites = 0; 
        uint64_t totalReads = 0; 

        for(auto intStruct : writeIterations)
        {
            totalWrites += intStruct.value;
        }

        for(auto intStruct : readIterations)
        {
            totalReads += intStruct.value;
        }


        double writeRate = totalWrites/static_cast<double>(opt.time);
        double readRate = totalReads/static_cast<double>(opt.time);

        double combinedRate = writeRate + readRate;

        printf("%s,%s,%s,%f,%d,%.3e,%.3e,%.3e\n",
                opt.name.c_str(),
                opt.lockType.c_str(),
                opt.csType.c_str(),
                opt.writeRatio,
                numThreads,
                writeRate,
                readRate,
                combinedRate);
    }
}


void chooseSection
(
    TestOptions& opt
)
{
    if(opt.csType == "empty")
    {
        opt.r_section = [](){};
        opt.w_section = [](){};
    }
    else if(opt.csType == "print")
    {
        opt.r_section = [](){};
        opt.w_section = []()
        {
            int cpuId =sched_getcpu();
            printf("Running on CPU: %d\n",cpuId);
        };
    }
    else
    {
        opt.r_section = [](){};
        opt.w_section = [](){};
    }
}

void chooseDist
(
    TestOptions& opt
)
{

    //true returns read, false returns write

    if(opt.distType == "random")
    {

    }
    else if(opt.distType == "pure-r")
    {
        opt.distribution = []()
        {
            return false;
        };
    }
    else if(opt.distType == "pure-w")
    {
        opt.distribution = []()
        {
            return true;
        };
    }
    else if(opt.distType == "fast-9-1")
    {
        opt.distribution = [=]()
        {
            static thread_local uint32_t counter = 0;
            counter += 1;
            if(counter >= 9)
            {
                counter -= 10;
                return true;
            }
            else
            {
                return false;
            }
        };
    }
    else if(opt.distType == "static")
    {
        int ratio = opt.writeRatio;
        opt.distribution = [=]()
        {
            static thread_local uint32_t counter = 0;
            counter += 1;

            return counter % 100 < ratio;
        };
    }
    else 
    {
        // by default return a 1 in 10 write, 9 in 10 read
        opt.distribution = []()
        {
            static thread_local int counter = 0;
            counter++;

            return counter % 10 == 0; 
        };
    }
}

void chooseGroupFunc
(
    TestOptions& opt
)
{
    std::function<uint32_t()> grpFunc;
    if(opt.groupFunc == "smt_grp1")
    {
        grpFunc = []()
        {
            int cpuSize = std::thread::hardware_concurrency();
            int cpuId = sched_getcpu();

            return cpuId % (cpuSize/2);
        };
    }
    MrwNaOptLock::setGroupFunction(grpFunc);
}

void runTest
(
    const TestOptions& opt
)
{
    if(opt.lockType == "mrw-opt")
    {
        rwThrptTest<MrwLockOpt>(opt);
    }
    else if(opt.lockType == "mrw")
    {
        rwThrptTest<MrwLock>(opt);
    }
    else if(opt.lockType == "crmr-w")
    {
        rwThrptTest<CrmrRwLock>(opt);
    }
    else if(opt.lockType == "crmr-r")
    {
        rwThrptTest<CrmrRwLockR>(opt);
    }
    else if(opt.lockType == "cpp-std")
    {
        rwThrptTest<BaseRwLock>(opt);
    }
    else
    {
        printf("Unknown lock type: %s\n",opt.lockType.c_str());
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

    parser.addOption("--distType",
            [&](const std::string& s)
            {
                test.distType = s;
            },true);

    parser.addOption("--groupFunc",
            [&](const std::string& s)
            {
                test.groupFunc = s;
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

    
    chooseDist(test);
    chooseSection(test);
    chooseGroupFunc(test);
    //runRwCorrectnessTestsForLock<MrwLockOpt>(5,{4,8,8,8,8,8,8,8,8,8,15},"MRW-OPT 7800x3d",true);
    //runRwCorrectnessTestsForLock<MrwLockOpt>(2,std::vector<int>(100,8),"MRW-OPT 7800x3d",true);
    //runRwCorrectnessTestsForLock<CrmrRwLock>(10,{4,8,15},"CRMR-WP 7800x3d",true);
    runTest(test);
}
