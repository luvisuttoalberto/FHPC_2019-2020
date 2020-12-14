#!/bin/bash
module load intel/18.4

cd Assignment02/

icc serial_time.c -o serial_time.x -lrt -std=c99 -fopenmp -DOUTPUT

export OMP_NUM_THREADS=20

export KMP_AFFINITY=granularity=fine,scatter

for max_lev in {27..10}; do
	./serial_time.x 1000000000 $max_lev >> serial_time.dat
done; 