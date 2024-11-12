#!/bin/bash

julia Generator/generator.jl spmv femlab
julia Generator/generator.jl spmv RMAT -N 10 -p 0.1

