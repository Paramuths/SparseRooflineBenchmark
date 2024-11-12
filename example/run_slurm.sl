#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=24
#SBATCH --exclusive
#SBATCH -t 12:00:00
#SBATCH --partition=lanka-v3
#SBATCH --qos=commit-main
#SBATCH --mem 102400

export OMP_NUM_THREADS=12

cd /data/scratch/paramuth/SparseRooflineBenchmark

./example/spmv -i data/1024-0.1 -o result/1024-0.1 -t 12
./example/spmv -i data/8192-0.1 -o result/8192-0.1 -t 12
./example/spmv -i data/1048576-3000000 -o result/1048576-3000000 -t 12
./example/spmv -i data/FEMLAB/poisson3Da -o result/FEMLAB/poisson3Da -t 12
./example/spmv -i data/FEMLAB/poisson3Db -o result/FEMLAB/poisson3Db -t 12

