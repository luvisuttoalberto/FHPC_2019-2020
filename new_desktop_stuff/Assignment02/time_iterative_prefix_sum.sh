#!/bin/bash
module load intel/18.4

cd Assignment02/

icc time_iterative_prefix_sum.c -o time_iterative_prefix_sum.x -lrt -std=c99 -fopenmp -DOUTPUT

export KMP_AFFINITY=granularity=fine,scatter

for p in {1..20}; do
	
	export OMP_NUM_THREADS=$p

	printf $"numero threads = "$p'\n' >> time_iterative_prefix_sum.dat

	for max_lev in {2..29}; do
		printf "max level = "$max_lev'\n' >> time_iterative_prefix_sum.dat
		./time_iterative_prefix_sum.x 1000000000 $max_lev >> time_iterative_prefix_sum.dat
	done;

done;

