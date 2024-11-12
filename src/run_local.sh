#!/bin/bash

julia Generator/generator.jl spmv femlab
julia Generator/generator.jl spmv RMAT -N 10 -p 0.1
julia Generator/generator.jl spmv RMAT -N 13 -p 0.1
julia Generator/generator.jl spmv RMAT2 -N 20 -p 3000000

