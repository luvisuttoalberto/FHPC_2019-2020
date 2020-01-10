#!/bin/bash
module load intel/18.4
cd FHPC_2019-2020/Assignment03/

icc dyn_op_mandelbrot.c -o dyn_op_mandelbrot.x -lrt -fopenmp -std=c99

export KMP_AFFINITY=granularity=fine,scatter

export OMP_NUM_THREADS=20

for fraction in {1..1000}; do
	./dyn_op_mandelbrot.x $fraction >> chunck_time.dat
done;