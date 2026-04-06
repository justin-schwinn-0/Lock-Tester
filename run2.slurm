#!/bin/bash
#SBATCH -J locktest             # Job name
#SBATCH -o outputs/locktest.o%j         # Output file (%j replaces with job ID)
#SBATCH -e locktest.e%j         # Error  file
#SBATCH -p PARTITION                # Partition (Lonestar6 default)
#SBATCH -N N                     # Number of nodes
#SBATCH -n N                     # Number of tasks (1 program)
#SBATCH --cpus-per-task=X       # THREADS requested (change this!)
#SBATCH -t 02:00:00              # Runtime (HH:MM:SS)
#SBATCH -A ASC25018
#SBATCH --mail-type=all    # Send email at begin and end of job
#SBATCH --mail-user=justinsch1029@gmail.com

# Set thread count in environment (important!)
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK
export NUM_THREADS=$SLURM_CPUS_PER_TASK

# Run your program
srun bash -c '
cd ${SLURM_SUBMIT_DIR}
./new-cs-tests/ARCH-test-${SLURM_PROCID}.sh
'
