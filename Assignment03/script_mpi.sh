cd FHPC_2019-2020/Assignment03/
module load intel/18.4

icc serial_mandelbrot.c -o serial_mandelbrot.x -std=c99 -lrt
/usr/bin/time ./serial_mandelbrot.x 1> serial.dat 2>&1
serial=$(cat serial.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)
module rm intel/18.4

module load openmpi
mpicc mpi_mandelbrot.c -o mpi_mandelbrot.x -lrt -std=c99

printf $"p"'\t'"w_c_p"'\t'"w_c_s"'\t'"elapsed"'\t'"serial"'\n' > mandelbrot.dat

for p in {2..20}; do
    
    /usr/bin/time mpirun -np $p ./mpi_mandelbrot.x 1> raw_mandelbrot.dat 2>&1
    

    w_c_p_s=$(cat raw_mandelbrot.dat | grep took | cut -f 3  -d " " | sort | tail -n 1 )
    

    if [ $p == 2 ]
    then
        export w_c_s_s="$(echo $w_c_p_s)"
    fi

    elapsed_s=$(cat raw_mandelbrot.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)
    printf $p'\t'$w_c_p_s'\t'$w_c_s_s'\t'$elapsed_s'\t'$serial'\n' >> mandelbrot.dat

done; rm raw_mandelbrot.dat serial.dat