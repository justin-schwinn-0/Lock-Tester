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
        return f"--csType \"{self.name}\" --t1 {self.t1} --t2 {self.t2} --t3 {self.t3}"

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
        print(f"{self.totalTime}s ({self.totalTime/60} min) across {len(self.tests)} tests")

    def writeTests(self, oFilePath):
        with open(oFilePath,"w") as f:
            f.write("#!/bin/bash")
            for t in self.tests:
                f.write("\n")
                f.write(t)
            
        
p = argparse.ArgumentParser(description="Runs an argument space of tests.")

p.add_argument("--threads", type=int,default=1,help="Max number of threads to run")
p.add_argument("--trials", type=int,default=1,help="trials per test")
p.add_argument("--seconds", type=int,default=1,help="seconds per test")
p.add_argument("--numNodes", type=int,default=1,help="number of nodes to split across")
p.add_argument("--nodeName", type=str,default=1,help="name of node")
p.add_argument("--outDir", type=str,default=1,help="Output Directory")

args = p.parse_args();

threadSpace = []

for i in range(8,args.threads+1,8):
    threadSpace.append(i)

ratioSpace = ["2.0","5.0","10.0","20.0"]

lockSpace = ["mrw-opt",
             "crmr-w",
             "cpp-std",
             "ck_tflock",
             "ck_pflock",
             "ck_rwlock"]

csSpace = [csSpot("n-mem-1",1,1),
           csSpot("n-mem-1K",1,1),
           csSpot("n-mem-1K",5,5),
           csSpot("n-mem-1G",1,1),
           csSpot("n-mem-1G",5,5),
           csSpot("empty",1,1)]

count = 0
groups = []

for i in range(0,args.numNodes):
    groups.append(TestGroup())

i = 0

name = args.nodeName
sec = args.seconds
trials = args.trials

for l in lockSpace:
    for c in csSpace:
        for t in threadSpace:
            for r in ratioSpace:
                count += args.trials
                testStr = f"{args.nodeName} {l} {c.getStr()} {t}"

                testStr = f"numactl --interleave=all ./build/LockTester --name {name} --time {sec} --threads {t} --lockType \"{l}\" --distType \"random\" --ratio {r} {c.getStr()} --trials {trials}"
                groups[i].addTest(testStr,args.seconds*args.trials)
                i = (i+1) % (len(groups))


print(f"{len(groups)} groups")

i = 0
for g in groups:
    g.print();
    g.writeTests(f"{args.outDir}/{args.nodeName}-test-{i}.sh")
    i+=1
