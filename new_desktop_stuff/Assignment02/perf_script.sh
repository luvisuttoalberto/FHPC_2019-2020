#!/bin/bash

module load intel/18.4

cd Assignment02/

icc 01_array_sum.c -o 01_array_sum.x -lrt -std=c99 -fopenmp -DOUTPUT

icc 04_touch_by_all.c -o 04_touch_by_all.x -lrt -std=c99 -fopenmp -DOUTPUT

export KMP_AFFINITY=granularity=fine,scatter

for p in {1..20}; do

    export OMP_NUM_THREADS=$p

    printf $"numero threads = "$p'\n' >> 01_perf_results.dat

    perf stat -e cache-misses,cycles,instructions ./01_array_sum.x 1000000000 2>&1 | grep cache -A 2 >> 01_perf_results.dat

done;

for p in {1..20}; do

    export OMP_NUM_THREADS=$p

    printf $"numero threads = "$p'\n' >> 04_perf_results.dat

	perf stat -e cache-misses,cycles,instructions ./04_touch_by_all.x 1000000000 2>&1 | grep cache -A 2 >> 04_perf_results.dat

done;