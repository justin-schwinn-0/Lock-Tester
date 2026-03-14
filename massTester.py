import os

name = "2x epyc milan test"
time = 10
threads = [8,16,32,40,48,56,64,72,80,88,96,104,112,128]
locks["mrw-opt","crmr-w"]

searchLimits = [5]
casLimits = [5]

def createCmd(name,time,l,t,s,c):
    cmd = f"./build/LockTester --name {name} --time {time} --threads {t} --x1 {s} --x2 {c} "
    cmd += f"--csType \"empty\" --lockType \"{l}\" --distType \"fast-9-1\""

    return cmd

#lengthOfTests = time * len(threads) * len(searchLimits) * len(casLimits)
#print(f"Test will take {lengthOfTests}s")

for t in threads:
    for l in locks
        cmd = createCmd(name,time,l,t,5,5)
        os.system(cmd);

