#!/bin/bash
module load intel/18.4

cd Assignment02/

export KMP_AFFINITY=granularity=fine,scatter

icc prefix_sum.c -o prefix_sum_serial.x -DOUTPUT -lrt -std=c99

icc iterative_prefix_sum.c -o iterative_prefix_sum.x -fopenmp -DOUTPUT -lrt -std=c99

icc new_iterative_prefix_sum.c -o new_iterative_prefix_sum.x -fopenmp -DOUTPUT -lrt -std=c99


printf $"p"'\t'"wall_clock_p"'\t'"elapsed"'\t'"serial"'\t'"wall_clock_s"'\n'> time_iterative.dat
printf $"p"'\t'"wall_clock_p"'\t'"elapsed"'\t'"serial"'\t'"wall_clock_s"'\n'> time_new.dat

printf $"threads"'\t'"sum"'\t'"avg_thread"'\t'"min_thread"'\n'> extra_iterative.dat
printf $"threads"'\t'"sum"'\t'"avg_thread"'\t'"min_thread"'\n'> extra_new.dat

/usr/bin/time ./prefix_sum_serial.x 1000000000 1> serial_iterative.dat 2>&1
/usr/bin/time ./prefix_sum_serial.x 1000000000 1> serial_new.dat 2>&1

serial_iterative=$(cat serial_iterative.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)
serial_new=$(cat serial_new.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)

for p in {1..20}; do
    export OMP_NUM_THREADS=$p

    /usr/bin/time ./iterative_prefix_sum.x 1000000000 1> raw_iterative.dat 2>&1
    /usr/bin/time ./new_iterative_prefix_sum.x 1000000000 1> raw_new.dat 2>&1

    wall_clock_iterative=$(cat raw_iterative.dat | grep Sum | cut -f 6 -d " ")
    wall_clock_new=$(cat raw_new.dat | grep Sum | cut -f 6 -d " ")

    if [ $p == 1 ]
    then
    export wk_siterative="$(echo $wall_clock_iterative)"
    export wk_snew="$(echo $wall_clock_new)"
    fi

    elapsed_iterative=$(cat raw_iterative.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)
    elapsed_new=$(cat raw_new.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)

    threads_iterative=$(cat raw_iterative.dat | grep omp | cut -f 4 -d " ")
    threads_new=$(cat raw_new.dat | grep omp | cut -f 4 -d " ")

    sum_iterative=$(cat raw_iterative.dat | grep Sum | cut -f 3 -d " " |  tr -d ,)
    sum_new=$(cat raw_new.dat | grep Sum | cut -f 3 -d " " |  tr -d ,)

    avg_thread_iterative=$(cat raw_iterative.dat | grep avg | cut -f 1 -d " " | tr -d "<,>" | head -1)
    avg_thread_new=$(cat raw_new.dat | grep avg | cut -f 1 -d " " | tr -d "<,>" | head -1)

    min_thread_iterative=$(cat raw_iterative.dat | grep min | cut -f 1 -d " " | tr -d "<,>" | head -1)
    min_thread_new=$(cat raw_new.dat | grep min | cut -f 1 -d " " | tr -d "<,>" | head -1)

    printf $p'\t'$wall_clock_iterative'\t'$elapsed_iterative'\t'$serial_iterative'\t'$wk_siterative'\n'>> time_iterative.dat
    printf $p'\t'$wall_clock_new'\t'$elapsed_new'\t'$serial_new'\t'$wk_snew'\n'>> time_new.dat

    printf $threads_iterative'\t'$sum_iterative'\t'$avg_thread_iterative'\t'$min_thread_iterative'\n'>> extra_iterative.dat
    printf $threads_new'\t'$sum_new'\t'$avg_thread_new'\t'$min_thread_new'\n'>> extra_new.dat

done; rm serial_iterative.dat serial_new.dat raw_iterative.dat raw_new.dat