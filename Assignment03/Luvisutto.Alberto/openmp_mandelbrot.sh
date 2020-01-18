cd FHPC_2019-2020/Assignment03/Luvisutto.Alberto/

module load intel/18.4

icc openmp_mandelbrot.c -o openmp_mandelbrot.x -std=c99 -lrt -fopenmp

export KMP_AFFINITY=granularity=fine,scatter

export OMP_NUM_THREADS=20

/usr/bin/time ./openmp_mandelbrot.x 

module rm intel/18.4