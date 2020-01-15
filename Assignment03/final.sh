cd FHPC_2019-2020/Assignment03/
module load intel/18.4

icc final_mandelbrot.c -o final_mandelbrot.x -lrt -std=c99 -fopenmp

export KMP_AFFINITY=granularity=fine,scatter

export OMP_NUM_THREADS=20

./final_mandelbrot.x

