import os
import argparse

class csSpot:
    name = ""
    t1 = -1
    t2 = -1
    t3 = -1

    def __init__(self,name,t1 = -1,t2 = -1,t3 = -1):
        self.name = name
        self.t1 = t1
        self.t2 = t2
        self.t3 = t3

    def getStr(self):
        return f"--csType {self.name} --t1 {self.t1} --t2 {self.t2} --t3 {self.t3}"

class TestGroup:
    tests = []
    totalTime = 0

    def __init__(self):
        self.tests = []
        self.totalTime = 0

    def addTest(self,test,time):
        self.tests.append(test)
        self.totalTime += time

    def print(self):
        print(f"{self.totalTime}s across {len(self.tests)} tests")
        for t in self.tests:
            print(f"{t}")
            
        
p = argparse.ArgumentParser(description="Runs an argument space of tests.")

p.add_argument("--threads", type=int,default=1,help="Max number of threads to run")
p.add_argument("--trials", type=int,default=1,help="trials per test")
p.add_argument("--seconds", type=int,default=1,help="seconds per test")
p.add_argument("--numNodes", type=int,default=1,help="number of nodes to split across")

args = p.parse_args();

threadSpace = [1]

for i in range(8,args.threads+1,8):
    threadSpace.append(i)

lockSpace = ["mrw-opt","c-rmr-w","cppstd","mrw-co-2"]

csSpace = [csSpot("n-mem-1G",1,1),
           csSpot("n-mem-1G",5,5),
           csSpot("n-mem-1K",1,1),
           csSpot("n-mem-1K",5,5),
           csSpot("empty")]

count = 0
groups = []

for i in range(0,args.numNodes):
    groups.append(TestGroup())

i = 0

for l in lockSpace:
    for c in csSpace:
        for t in threadSpace:
            count += args.trials
            testStr = f"{l} {c.getStr()} {t}"
            groups[i].addTest(testStr,args.seconds*args.trials)
            i = (i+1) % (len(groups))

totalSeconds = count * args.seconds

secondsPerNode = totalSeconds / 900

print(len(lockSpace))
print(len(csSpace))
print(len(threadSpace))
print(f"total time: {totalSeconds}")
print(f"seconds per node: {secondsPerNode}")

print(f"{len(groups)} groups")

for g in groups:
    g.print()
