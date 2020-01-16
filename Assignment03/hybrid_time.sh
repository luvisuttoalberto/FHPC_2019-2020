cd FHPC_2019-2020/Assignment03/
module load openmpi/1.8.3/intel/14.0
module load impi-trial/5.0.1.035

mpiicc hybrid_mandelbrot.c -o hybrid_mandelbrot.x -std=c99 -fopenmp

export I_MPI_JOB_RESPECT_PROCESS_PLACEMENT=off
export OMP_NUM_THREADS=5
export OMP_PLACES=cores
export OMP_PROC_BIND=spread

/usr/bin/time mpiexec.hydra -ppn 4 ./hybrid_mandelbrot.x