#!/bin/bash
module load intel/18.4

cd Assignment02/

icc time_overhead.c -o time_overhead.x -lrt -std=c99 -fopenmp -DOUTPUT

export OMP_NUM_THREADS=20

export KMP_AFFINITY=granularity=fine,scatter

./time_overhead.x 1000000000 >> overhead_time.dat
