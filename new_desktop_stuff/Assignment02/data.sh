#!/bin/bash
module load intel/18.4

export KMP_AFFINITY=granularity=fine,scatter

printf $"p"'\t'"wall_clock_p"'\t'"elapsed"'\t'"serial"'\t'"wall_clock_s"'\n'> time_01.dat
printf $"p"'\t'"wall_clock_p"'\t'"elapsed"'\t'"serial"'\t'"wall_clock_s"'\n'> time_04.dat

printf $"threads"'\t'"sum"'\t'"avg_thread"'\t'"min_thread"'\n'> extra_01.dat
printf $"threads"'\t'"sum"'\t'"avg_thread"'\t'"min_thread"'\n'> extra_04.dat

/usr/bin/time ./01_array_sum_serial.x 1000000000 1> serial_01.dat 2>&1
/usr/bin/time ./04_touch_by_all_serial.x 1000000000 1> serial_04.dat 2>&1

serial_01=$(cat serial_01.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)
serial_04=$(cat serial_04.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)

for p in {1..20}; do
    export OMP_NUM_THREADS=$p

    /usr/bin/time ./01_array_sum.x 1000000000 1> raw_01.dat 2>&1
    /usr/bin/time ./04_touch_by_all.x 1000000000 1> raw_04.dat 2>&1

    wall_clock_01=$(cat raw_01.dat | grep Sum | cut -f 6 -d " ")
    wall_clock_04=$(cat raw_04.dat | grep Sum | cut -f 6 -d " ")

    if [ $p == 1 ]
    then
    export wk_s01="$(echo $wall_clock_01)"
    export wk_s04="$(echo $wall_clock_04)"
    fi

    elapsed_01=$(cat raw_01.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)
    elapsed_04=$(cat raw_04.dat | grep elapsed | cut -f 3 -d " " | sed 's/[a-zA-Z]//g' | tr -d :)

    threads_01=$(cat raw_01.dat | grep omp | cut -f 4 -d " ")
    threads_04=$(cat raw_04.dat | grep omp | cut -f 4 -d " ")

    sum_01=$(cat raw_01.dat | grep Sum | cut -f 3 -d " " |  tr -d ,)
    sum_04=$(cat raw_01.dat | grep Sum | cut -f 3 -d " " |  tr -d ,)

    avg_thread_01=$(cat raw_01.dat | grep avg | cut -f 1 -d " " | tr -d "<,>" | head -1)
    avg_thread_04=$(cat raw_04.dat | grep avg | cut -f 1 -d " " | tr -d "<,>" | head -1)

    min_thread_01=$(cat raw_01.dat | grep min | cut -f 1 -d " " | tr -d "<,>" | head -1)
    min_thread_04=$(cat raw_04.dat | grep min | cut -f 1 -d " " | tr -d "<,>" | head -1)

    printf $p'\t'$wall_clock_01'\t'$elapsed_01'\t'$serial_01'\t'$wk_s01'\n'>> time_01.dat
    printf $p'\t'$wall_clock_04'\t'$elapsed_04'\t'$serial_04'\t'$wk_s04'\n'>> time_04.dat

    printf $threads_01'\t'$sum_01'\t'$avg_thread_01'\t'$min_thread_01'\n'>> extra_01.dat
    printf $threads_04'\t'$sum_04'\t'$avg_thread_04'\t'$min_thread_04'\n'>> extra_04.dat

done; rm serial_01.dat serial_04.dat raw_01.dat raw_04.dat