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
#include "CrmrRwLockCo.h"
#include "MrwLock.h"
#include "MrwLockOpt.h"
#include "CrmrRwLockR.h"
#include "MrwLockCO.h"
#include "ManagedMrwCo.h"

#include "CkLocks.h"

#include "GroupRwCohort.h"
#include "CohortFunctions.h"

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

// about 1G
std::vector<uint8_t> bigVec1G = std::vector<uint8_t>(1 << 30,9);
std::vector<uint8_t> bigVec1K = std::vector<uint8_t>(1 << 10,9);
std::vector<uint8_t> bigVec1K_2 = std::vector<uint8_t>(1 << 10,9);

uint64_t singleVal = 0;

struct TestOptions
{
    int time = -1;
    int threads = -1;
    std::string name = "";
    std::string lockType = "";
    std::string distType = "";
    std::string csType = "";
    std::string groupFunc = "";
    float writeRatio = 10.0;
    bool verbose = false;
    int x1 = -1;
    int x2 = -1;
    int x3 = -1;
    int t1 = -1;
    int t2 = -1;
    int t3 = 0;
    int trials = 1;
    std::function<bool()> distribution;
    std::function<void()> w_section;
    std::function<void()> r_section;
};

struct ThreadLocals
{
    std::mt19937 gen;

    ThreadLocals()
    {
        int seed = std::chrono::system_clock::now().time_since_epoch().count();
        gen = std::mt19937(seed);
    }

    int intDistRand(int min, int max)
    {
        std::uniform_int_distribution<> dist(min,max);

        return dist(gen);
    }

    double realDistRand(double min, double max)
    {
        std::uniform_real_distribution<> dist(min,max);

        return dist(gen);
    }
};

static thread_local ThreadLocals tl = ThreadLocals();

struct Aligned64bUint
{
    alignas(64) uint64_t value;
};

#pragma GCC push_options
#pragma GCC optimize ("O0")
template<class RwLock>
void rwThrptTest
(
    const TestOptions& opt
)
{
    using namespace std::chrono_literals;
    int maxThreads = std::thread::hardware_concurrency();

    auto dist = opt.distribution;
    auto w_section = opt.w_section;
    auto r_section = opt.r_section;
    auto nc_section = [&]()
    {
        static thread_local int ncTotal = 0;
        

        for(int i = 0; i < opt.t3; i++)
        {
            int z = tl.intDistRand(0,bigVec1K_2.size());
            ncTotal += bigVec1K_2[z];
        }
    };
    int numThreads = opt.threads;
    int x1 = opt.x1;
    int x2 = opt.x2;
    int x3 = opt.x3;

    RwLock lock;

    bool startBarrier = false;

    bool continueFlag = true;

    auto thread = [&](Aligned64bUint& wItrs, Aligned64bUint& rItrs)
    {
        while(!startBarrier);

        while(continueFlag)
        {
            nc_section();

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

    printf("%s,%s,%s,%s,%.2f,%d,%d,%d,%d,%d,%d,%d,%.3e,%.3e,%.3e\n",
            opt.name.c_str(),
            opt.lockType.c_str(),
            opt.csType.c_str(),
            opt.distType.c_str(),
            opt.writeRatio,
            numThreads,
            x1,x2,x3,
            opt.t1,opt.t2,opt.t3,
            writeRate,
            readRate,
            combinedRate);
}
#pragma GCC pop_options


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
    else if(opt.csType == "n-mem-1G")
    {
        //thread will access n random bytes of memory,
        // from a huge vector.
        // the size of the vector makes it unlikley that the
        // value is stored in a cache,
        int rtrials = opt.t1; 
        int wtrials = opt.t2; 
        opt.r_section = [=,&bigVec1G,&tl]()
        {
            static thread_local int total = 0;

            for(int i = 0; i < rtrials; i++)
            {
                int z = tl.intDistRand(0,bigVec1G.size());
                total += bigVec1G[z];
            }
        };
        opt.w_section = [=,&bigVec1G,&tl]()
        {
            static thread_local int total = 0;

            for(int i = 0; i < wtrials; i++)
            {
                int z = tl.intDistRand(0,bigVec1G.size());
                total += bigVec1G[z];
                bigVec1G[z] = total;
            }
        };

    }
    else if(opt.csType == "n-mem-1K")
    {
        //thread will access n random bytes of memory,
        // from a huge vector.
        // the size of the vector makes it unlikley that the
        // value is stored in a cache,
        int rtrials = opt.t1; 
        int wtrials = opt.t2; 
        opt.r_section = [=,&bigVec1K]()
        {
            static thread_local int total = 0;

            for(int i = 0; i < rtrials; i++)
            {
                int z = tl.intDistRand(0,bigVec1K.size());
                total += bigVec1K[z];
            }
        };
        opt.w_section = [=,&bigVec1K]()
        {
            static thread_local int total = 0;

            for(int i = 0; i < wtrials; i++)
            {
                int z = tl.intDistRand(0,bigVec1K.size());
                total += bigVec1K[z];
                bigVec1K[z] = total;
            }
        };

    }
    else if( opt.csType == "n-mem-1")
    {
        int rtrials = opt.t1; 
        int wtrials = opt.t2; 
        opt.r_section = [=,&singleVal]()
        {
            static thread_local int total = 0;

            for(int i = 0; i < rtrials; i++)
            {
                total += singleVal;
            }
        };
        opt.w_section = [=,&singleVal]()
        {
            static thread_local int total = 0;

            for(int i = 0; i < wtrials; i++)
            {
                total += singleVal;
                singleVal = total;
            }
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
    int ratio = opt.writeRatio;

    if(opt.distType == "random")
    {
        opt.distribution = [&]()
        {
            return tl.realDistRand(0,100) < ratio;
        };
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
            if(counter > 9)
            {
                counter = 0;
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

using MrwCo2 = GroupRwCohort<FetchAndIncLock,
                    MrwLockCO,
                    NumaN_NHT<2>,
                    2>;

using MrwCo8 = GroupRwCohort<FetchAndIncLock,
                    MrwLockCO,
                    NumaN_NHT<8>,
                    8>;

using MrwCo16 = GroupRwCohort<FetchAndIncLock,
                    MrwLockCO,
                    NumaN_NHT<16>,
                    16>;

using CrmrwCo2 = GroupRwCohort<FetchAndIncLock,
                    CrmrRwLockCo,
                    NumaN_NHT<2>,
                    2>;

using CrmrwCo8 = GroupRwCohort<FetchAndIncLock,
                    CrmrRwLockCo,
                    NumaN_NHT<8>,
                    8>;

using CrmrwCo16 = GroupRwCohort<FetchAndIncLock,
                    CrmrRwLockCo,
                    NumaN_NHT<16>,
                    16>;

using MrwCo8_HT = GroupRwCohort<FetchAndIncLock,
                    MrwLockCO,
                    NumaN_NHT<8>,
                    8>;

void runTest
(
    const TestOptions& opt
)
{
    int s = opt.x1;
    int c = opt.x2;
    int defVal = (std::thread::hardware_concurrency() / 4) +1;

    if(s == -1)
    {
        s = defVal;
    }
    if(c == -1)
    {
        c = defVal;
    }

    MrwLockOpt::setNodeSearchLimit(s);
    MrwLockOpt::setCasAttemptLimit(c);

    MrwLockCO::setNodeSearchLimit(s);
    MrwLockCO::setCasAttemptLimit(c);

    if(opt.lockType == "mrw-opt")
    {
        rwThrptTest<MrwLockOpt>(opt);
    }
    else if(opt.lockType == "mmrwco")
    {
        rwThrptTest<ManagedMrwCo>(opt);
    }
    else if(opt.lockType == "crmr-w")
    {
        rwThrptTest<CrmrRwLock>(opt);
    }
    else if(opt.lockType == "crmr-r")
    {
        rwThrptTest<CrmrRwLockR>(opt);
    }
    else if(opt.lockType == "mrw-co-2")
    {
        rwThrptTest<MrwCo2>(opt);
    }
    else if(opt.lockType == "mrw-co-8")
    {
        rwThrptTest<MrwCo8>(opt);
    }
    else if(opt.lockType == "mrw-co-16")
    {
        rwThrptTest<MrwCo16>(opt);
    }
    else if(opt.lockType == "crmr-w-co-2")
    {
        rwThrptTest<CrmrwCo2>(opt);
    }
    else if(opt.lockType == "crmr-w-co-8")
    {
        rwThrptTest<CrmrwCo8>(opt);
    }
    else if(opt.lockType == "crmr-w-co-16")
    {
        rwThrptTest<CrmrwCo16>(opt);
    }
    else if(opt.lockType == "ck-tflock")
    {
        rwThrptTest<CkTfLock>(opt);
    }
    else if(opt.lockType == "ck-rwlock")
    {
        rwThrptTest<CkRwLock>(opt);
    }
    else if(opt.lockType == "ck-pflock")
    {
        rwThrptTest<CkPfLock>(opt);
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

    
    //runRwCorrectnessTestsForLock<CkTfLock>(1,{4,8,15},"CK-TfLock",true);
    //runRwCorrectnessTestsForLock<CkRwLock>(1,{4,8,15},"CK-RwLock",true);
    //runRwCorrectnessTestsForLock<CkPfLock>(1,{4,8,15},"CK-PfLock",true);
    runRwCorrectnessTestsForLock<MrwLockOpt>(1,{4,8,15},"MRW",true);
    runRwCorrectnessTestsForLock<CrmrRwLock>(1,{4,8,15},"CRMR",true);

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

    parser.addOption("--threads",
            [&](const std::string& s)
            {
                test.threads = Utils::strToInt(s);
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

    parser.addOption("--x1",
            [&](const std::string& s)
            {
                test.x1 = Utils::strToInt(s);
            },true);

    parser.addOption("--x2",
            [&](const std::string& s)
            {
                test.x2 = Utils::strToInt(s);
            },true);

    parser.addOption("--x3",
            [&](const std::string& s)
            {
                test.x3 = Utils::strToInt(s);
            },true);

    parser.addOption("--t1",
            [&](const std::string& s)
            {
                test.t1 = Utils::strToInt(s);
            },true);

    parser.addOption("--t2",
            [&](const std::string& s)
            {
                test.t2 = Utils::strToInt(s);
            },true);

    parser.addOption("--t3",
            [&](const std::string& s)
            {
                test.t3 = Utils::strToInt(s);
            },true);

    parser.addOption("--trials",
            [&](const std::string& s)
            {
                test.trials = Utils::strToInt(s);
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
    else if(test.threads <= 0)
    {
        printf("Test must have positive number of threads!\n");
        std::exit(1);
    }

    
    chooseDist(test);
    chooseSection(test);

    //runRwCorrectnessTestsForLock<CrmrwCo2>(1,{4,8,15},"CrmrRwLockCo2 7800x3d",true);
    //runRwCorrectnessTestsForLock<CrmrwCo8>(1,{4,8,15},"CrmrRwLockCo8 7800x3d",true);
    //runRwCorrectnessTestsForLock<CrmrwCo16>(1,{4,8,15},"CrmrRwLockCo16 7800x3d",true);
    //runRwCorrectnessTestsForLock<MrwLockOpt>(2,{4,8,8,8,8,8,8,8,8,8,15},"MrwLockOpt 7800x3d",true);
    //runRwCorrectnessTestsForLock<MrwLockOpt>(2,std::vector<int>(100,8),"MRW-OPT 7800x3d",true);
    //runRwCorrectnessTestsForLock<CrmrRwLock>(10,{4,8,15},"CRMR-WP 7800x3d",true);
    for(int i =0; i < test.trials; i++)
    {
        runTest(test);
    }
}
