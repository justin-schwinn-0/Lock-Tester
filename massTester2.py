import os
import sys

time = 3

searchLimits = [1,2,5,10,20]
casLimits = [1,2,5,10,20]

def createCmd(name,time,t,s,c):
    cmd = f"./build/LockTester --name {name} --time {time} --threads {t} --x1 {s} --x2 {c} "
    cmd += "--csType \"empty\" --lockType \"mrw-opt\" --distType \"fast-9-1\""

    return cmd

#lengthOfTests = time * len(threads) * len(searchLimits) * len(casLimits)
#print(f"Test will take {lengthOfTests}s")

name = sys.argv[1]
t = sys.argv[2]

for s in searchLimits:
    for c in casLimits:
        cmd = createCmd(name,time,t,s,c)
        os.system(cmd);
