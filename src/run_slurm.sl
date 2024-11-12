#!/bin/bash
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=24
#SBATCH --exclusive
#SBATCH -t 12:00:00
#SBATCH --partition=lanka-v3
#SBATCH --qos=commit-main
#SBATCH --mem 102400
cd /data/scratch/paramuth/SparseRooflineBenchmark/src
julia Generator/generator.jl spmv femlab
julia Generator/generator.jl spmv RMAT -N 10 -p 0.1

