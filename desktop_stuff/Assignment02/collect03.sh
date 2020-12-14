module load intel/18.4
cd Assignment02/

icc prefix_sum.c -o prefix_sum_serial.x -std=c99 -lrt
icc -fopenmp new_iterative_prefix_sum.c -o new_iterative_prefix_sum.x -std=c99 -lrt

export KMP_AFFINITY=granularity=fine,scatter

printf $"p"'\t'"w_c_p"'\t'"w_c_s"'\t'"elapsed"'\t'"serial"'\n' > output_3.dat

/usr/bin/time ./prefix_sum_serial.x 1000000000 1> serial_3.dat 2>&1
serial=$(cat serial_3.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)

for p in {1..20}; do 
    export OMP_NUM_THREADS=$p

    /usr/bin/time ./new_iterative_prefix_sum.x 1000000000 1> raw_3.dat 2>&1

    w_c_p=$(cat raw_3.dat | grep took | cut -f 3  -d " ")
    if [ $p == 1 ] 
    then
    export w_c_s="$(echo $w_c_p)"
    fi 
    elapsed=$(cat raw_3.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)
    printf $p'\t'$w_c_p'\t'$w_c_s'\t'$elapsed'\t'$serial'\n' >> output_3.dat

done; rm serial_3.dat raw_3.dat