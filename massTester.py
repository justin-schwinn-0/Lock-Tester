import os

name = "7800x3d_configs"
time = 3
threads = [4,8,12,16]

searchLimits = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]
casLimits = [1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16]

def createCmd(name,time,t,s,c):
    cmd = f"./build/LockTester --name {name} --time {time} --threads {t} --x1 {s} --x2 {c} "
    cmd += "--csType \"empty\" --lockType \"mrw-opt\" --distType \"fast-9-1\""

    return cmd

#lengthOfTests = time * len(threads) * len(searchLimits) * len(casLimits)
#print(f"Test will take {lengthOfTests}s")

for t in threads:
    for s in searchLimits:
        for c in casLimits:
            cmd = createCmd(name,time,t,s,c)
            os.system(cmd);
