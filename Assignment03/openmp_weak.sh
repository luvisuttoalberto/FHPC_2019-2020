cd FHPC_2019-2020/Assignment03
module load intel/18.4

icc serial_mandelbrot.c -o serial_mandelbrot.x -std=c99 -lrt
icc dyn_op_mandelbrot.c -o dyn_op_mandelbrot.x -std=c99 -fopenmp -lrt

/usr/bin/time ./serial_mandelbrot.x 1> serial.dat 2>&1
serial=$(cat serial.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)

export KMP_AFFINITY=granularity=fine,scatter

printf $"p"'\t'"elapsed"'\t'"serial"'\n' > weak_openmp.dat

for p in {1..20}; do

    export OMP_NUM_THREADS=$p
   
    root="$(echo "scale=3; sqrt($p)" | bc)"

    n_x=$(echo $root*8000 | bc)
    n_y=$(echo $root*4000 | bc)
       
    /usr/bin/time ./dyn_op_mandelbrot.x $n_x $n_y -2.75 1 1.25 -1 255 1> raw_openmp_weak.dat 2>&1

    elapsed=$(cat raw_openmp_weak.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)

    printf $p'\t'$elapsed'\t'$serial'\n' >> weak_openmp.dat

done; rm serial.dat raw_openmp_weak.dat