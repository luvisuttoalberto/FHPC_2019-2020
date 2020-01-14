cd FHPC_2019-2020/Assignment03/
module load intel/18.4

icc serial_mandelbrot.c -o serial_mandelbrot.x -std=c99 -lrt

/usr/bin/time ./serial_mandelbrot.x 1> serial.dat 2>&1
serial=$(cat serial.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)

module rm intel/18.4
module load openmpi

mpicc mpi_mandelbrot.c -o mpi_mandelbrot.x -std=c99 -lrt

printf $"p"'\t'"elapsed"'\t'"serial"'\n' > weak_mpi.dat

for p in {2..20}; do

    root="$(echo "scale=3; sqrt($p)" | bc)"

    n_x=$(echo $root*8000 | bc)
    n_y=$(echo $root*4000 | bc)
       
    /usr/bin/time mpirun -np $p ./mpi_mandelbrot.x $n_x $n_y -2.75 1 1.25 -1 255 1> raw_weak_mpi.dat 2>&1

    elapsed=$(cat raw_weak_mpi.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)

    printf $p'\t'$elapsed'\t'$serial'\n' >> weak_mpi.dat

done; rm serial.dat raw_weak_mpi.dat
