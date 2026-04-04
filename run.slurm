#!/bin/bash
#SBATCH -J locktest             # Job name
#SBATCH -o outputs/locktest.o%j         # Output file (%j replaces with job ID)
#SBATCH -e locktest.e%j         # Error  file
#SBATCH -p PARTITION                # Partition (Lonestar6 default)
#SBATCH -N N                     # Number of nodes
#SBATCH -n N                     # Number of tasks (1 program)
#SBATCH --cpus-per-task=128       # THREADS requested (change this!)
#SBATCH -t 00:30:00              # Runtime (HH:MM:SS)
#SBATCH -A ASC25018

# Set thread count in environment (important!)
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export NUM_THREADS=$SLURM_CPUS_PER_TASK

# Run your program
srun bash -c '
cd ${SLURM_SUBMIT_DIR}
./node-cmd-files/ARCH-test-${SLURM_PROCID}.sh
'
