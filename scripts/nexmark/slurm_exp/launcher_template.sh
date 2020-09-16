#!/bin/bash -l
`#'SBATCH -N NOF_NODES
`#'SBATCH --ntasks-per-node=NOF_TASKS_PER_NODE
`#'SBATCH --cpus-per-task=NOF_CORES_PER_TASK
`#'SBATCH --time=OBSERVATION_TIME
`#'SBATCH -p batch
`#'SBATCH --qos qos-batch
`#'SBATCH -C broadwell
`#'SBATCH --mem=NODE_MEM

`#'SBATCH -J JOB_NAME
`#'SBATCH -o slurm_exp/observations/QUERY-NOF_NODES-NOF_DATAFLOWS-THROUGHPUT.csv
`#'SBATCH -e slurm_exp/observations/QUERY-NOF_NODES-NOF_DATAFLOWS-THROUGHPUT.err

#===============================================================================
# documentation : https://hpc.uni.lu/users/docs/slurm_launchers.html
#===============================================================================

module load toolchain/intel
module load devel/CMake/3.13.3-GCCcore-8.2.0
module load compiler/GCC/8.2.0-2.31.1

srun --overcommit -n NOF_DATAFLOWS EXECUTABLE QUERY THROUGHPUT
