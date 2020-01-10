cd FHPC_2019-2020/Assignment03/
module load intel/18.4

icc serial_mandelbrot.c -o serial_mandelbrot.x -std=c99 -lrt
icc op_mandelbrot.c -o op_mandelbrot.x -std=c99 -fopenmp -lrt
icc dyn_op_mandelbrot.c -o dyn_op_mandelbrot.x -std=c99 -fopenmp -lrt

export KMP_AFFINITY=granularity=fine,scatter

printf $"p"'\t'"w_c_p"'\t'"w_c_s"'\t'"elapsed"'\t'"serial"'\n' > parallel_static.dat
printf $"p"'\t'"w_c_p"'\t'"w_c_s"'\t'"elapsed"'\t'"serial"'\n' > parallel_dynamic.dat

/usr/bin/time ./serial_mandelbrot.x 1> serial.dat 2>&1
serial=$(cat serial.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)

for p in {1..20}; do
    export OMP_NUM_THREADS=$p

    /usr/bin/time ./op_mandelbrot.x 1> raw_parallel_static.dat 2>&1
    /usr/bin/time ./dyn_op_mandelbrot.x 1> raw_parallel_dynamic.dat 2>&1

    w_c_p_s=$(cat raw_parallel_static.dat | grep took | cut -f 3  -d " ")
    w_c_p_d=$(cat raw_parallel_dynamic.dat | grep took | cut -f 3  -d " ")

    if [ $p == 1 ]
    then
        export w_c_s_s="$(echo $w_c_p_s)"
        export w_c_s_d="$(echo $w_c_p_d)"
    fi

    elapsed_s=$(cat raw_parallel_static.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)
    elapsed_d=$(cat raw_parallel_dynamic.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)

    printf $p'\t'$w_c_p_s'\t'$w_c_s_s'\t'$elapsed_s'\t'$serial'\n' >> parallel_static.dat
    printf $p'\t'$w_c_p_d'\t'$w_c_s_d'\t'$elapsed_d'\t'$serial'\n' >> parallel_dynamic.dat

done; rm raw_parallel_static.dat raw_parallel_dynamic.dat serial.dat