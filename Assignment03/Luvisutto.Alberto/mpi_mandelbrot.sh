cd FHPC_2019-2020/Assignment03/Luvisutto.Alberto/

module load openmpi
mpicc mpi_mandelbrot.c -o mpi_mandelbrot.x -lrt -std=c99

/usr/bin/time mpirun -np 20 ./mpi_mandelbrot.x

module rm openmpi