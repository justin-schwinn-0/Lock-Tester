
build/LockTester \
    --name "7800x3d" \
    --time 2 \
    --threads 16 \
    --csType "n-mem-1" \
    --lockType "mrw-opt" \
    --distType "random" \
    --ratio "10.0" \
    --t1 5 \
    --t2 5 \
    --t3 10 \
    --trials 5 \
    -v
