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

./example/spmv -i data -o result -t 12
./example/spmv -i data/FEMLAB/poisson3Da -o result/FEMLAB/poisson3Da -t 12

