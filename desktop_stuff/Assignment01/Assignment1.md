### Section 0: (warm-up)

1. Compute Theoretical Peak performance for your laptop/desktop:

   The theoretical Peak performance is calculated with the following formula:
   $$
   cores \times frequency \times FLOPs/cycle
   $$
   

|        | Your model | CPU                         | Frequency                     | number  of core | Peak performance |
| ------ | ---------- | --------------------------- | ----------------------------- | --------------- | ---------------- |
| Laptop | Asus F556U | Intel(R) Core (TM) i7-6500U | 3.1 GHz (Max Turbo Frequency) | 2               | 24.8 GFlops/s    |

2. Compute sustained Peak performance for your cell-phone:

|            | Your model | sustained performance | size of matrix | Peak performance | memory |
| ---------- | ---------- | --------------------- | -------------- | ---------------- | ------ |
| SmartPhone | Honor 10   | 900 MFlops/s          | 4000           | 18.8 GFlops/s    | 4 MB   |

3. Find out in which year  your cell phone/laptop  could have been in top1% of Top500:

|            | Your model | performance   | Top500 year | Position in the TOP500 | HPC system in that position            | number of processors (TOP500) |
| ---------- | ---------- | ------------- | ----------- | ---------------------- | -------------------------------------- | ----------------------------- |
| SmartPhone | Honor 10   | 18.8 GFlops/s | 1993        | 7th                    | CM-5/256 Thinking Machines Corporation | 256                           |
| Laptop     | Asus F556U | 24.8 GFlops/s | 1993        | 5th                    | SX-3/44R NEC                           | 4                             |

The position of the smartphone in the 1993 TOP500 was inserted in the table even if it wasn't in the top 1% of the list, just to give an idea of the theoretical peak performance.

### Section 1: Theoretical model

- devise a performance model for a simple parallel algorithm: sum of N numbers

  - Serial Algorithm : n-1 operations 

    $T_{serial}= N*T_{comp} $

    $T_{comp}$= *time to compute a floating point operation*

  - Parallel Algorithm : master-slave

    - read N and distribute N to P-1  slaves ===>  $T_{read} + (P-1)\times T_{comm}$ 
    $T_{comm}$ = *time  each processor takes to communicate one message, i.e. latency..*
      $T_{read}$   = *time  master takes to read* 
  
    - N/P sum over each processors (including master)  ===> $T_{comp}\times N/P$

    - Slaves send partial sum  ===>   $(P-1)T_{comm}$

    - Master performs  one final sum ===>  $(P-1) \times T_{comp}$

      the final model:    $T_p=   T_{comp}\times (P -1 + N/P)  + T_{read} + 2(P-1)\times T_{comm}  $

- compute scalability curves for such algorithm and make some plots

  - assumptions:

    - $T_{comp} =2 \times 10^{-9}$
    - $T_{read}= 1 \times 10^{-4}$
    - $T_{comm}= 1 \times 10^{-6}$

    Play with some value of N and plot against P  (with P ranging from 1 to 1000)


In the following plot I considered four different sizes of N, that can be found in the label of the graph.

![]()

- Comment on them:

  - For which values of N do you see the algorithm scaling ? $10^6$ more or less, with $10^5$ we still are near the unity. The algorithm starts scaling shortly after $10^5$ (around $2*10^5$).

  - For which values of P does the algorithm produce the best results ? For the n considered ($10^6$ , $10^7$, $5*10^7$,  $10^8$) we have the best results for p = 32, 100, 223, 316 respectively.

  - Can you try to modify the algorithm sketched above to increase its scalability ? 

    (hints: try to think a  better communication  algorithm) 
  
    If at the beginning i make all the process read N, like in Mpi_pi, does this count as only one Read? Or for the fact that the reading is coordinated by the OS it all happens consequentially and the processes can't read simultaneously? Prova eventualmente a fare un programma in cui il master legge e il resto riceve in broadcast; in questo modo si avrebbe Tread + Tcomm]
    
    
    
    I can insert the collective operation "Reduce" so that the communication part of the algorithm goes from $(P-1)\times T_{comm}$ to just $T_{comm}$ (?????)
    
    In this way the final model is $T_p=   T_{comp}\times (P -1 + N/P)  + T_{read} + 2\times T_{comm}  $
    
    
  

## Section 2 : play with MPI program

### 2.1:  compute strong scalability of a mpi_pi.c program

The application we are using here is a toy application: a Monte-Carlo integration to compute PI; The idea is to have a circle inscribed inside a square of unit length. The ratio between the area of the square (1) and the circle (pi/4) is $\pi/4$. Therefore, if we randomly choose N points inside the square, on average, only `M=N*pi/4` points will belong to the circle. From the last relation we can estimate pi. We provide a basic serial implementation of the algorithm ( program pi.c ) and we also give a parallel MPI implementation ( mpi_pi.c ) that computes PI by the same algorithm using MPI to compute the algorithm in parallel. Your exercise is to see how well this application scales up to the total number of cores of one node. You can modify the codes we have given, or you can write your own.

Steps to do:

- Compile the serial and parallel version.
- Determine the CPU time required to calculate PI with the serial calculation using 1000000 (10 millions) iterations (stone throws). Make sure that this is the actual run time and does not include any system time.

Hint: use /usr/bin/time command to time all the applications

- Get the MPI code running for the same number of iterations.

The parallel code writes walltime for all the processor involved. Which of these times do you want to consider to estimate the parallel time ? Ideally the master process time, because he needs to wait for all the local_sum of the other processes and therefore will be the final one. But I observed that this is not always true; sometimes the walltime of the master process is smaller than the time of some other processes; I can only explain this with some operations between the end of the receive call of the master and the capture of the walltime.

- First let us do some running that constitutes a strong scaling test.

This means keeping the problem size constant, or in other words, keeping Niter = 10 millions. Start by running the MPI code with only one processor doing the numerical computation. A comparison of this to the serial calculation gives you some idea of the overhead associated with MPI. Again what time do you consider here ? In the MPI code the user one, which corresponds to the walltime of the only process involved; in the serial code the user time. 

- Keeping Niter = 10 millions, run the MPI code for 2, 4, 8 and 16 and 20 MPI processs.

In principle, all that needs to be done is to run the program multiple times, changing the -np argument to mpirun each time. Such a study could be as simple as the following bash for loop:

```
 for procs in 1 2 4 8 16 32 64 ; do
 time mpirun -np ${procs} my-program the-input
 done
```

A real study is of course a little bit more complicated because we need to get the performance of each of those runs. Furthermore we run on Ulysses (or any other HPC resources) and then we need to submit jobs.

In any case it is important to script your job executions, so that you can quickly measure scalability. So in this part of the lab you need to implement a strong scaling script. Script will be implemented by means of bash scripting, or if you prefer you can use python as well.

- Make a plot of run time versus number of nodes from the data you have collected.
- Strong scalability here would yield a straight line graph. Comment on your results. Repeat the work playing with a large dataset.
- Provide a final plot with at least 3 different size and for each of the size report and comment your final results.

### 2.2: identify a model for the parallel overhead

In this section you are asked to identify a simple model that estimate the parallel overhead of our simple program as a function of the number of processor. We ask here to extend the simple model discussed previously and adapt/modify to the present program.

To do this please review carefully all the data collected so far and identify which is the indirect method to measure the parallel overhead from the output of the program.


### 2.3: weak scaling 

- Now let us do some running that constitutes a weak scaling test.

This means increasing the problem size simultaneously with the number of nodes being used. In the present case, increasing the number of iterations, Niter.
- Record the run time for each number of nodes and make a plot of the run time versus number of computing nodes.

- Weak scaling would imply that the runtime remains constant as the problem size and the number of compute nodes increase in proportion. Modify your scripts to rapidly collect numbers for the weak scalability tests for different number of moves.

- Plot on the same graph the efficiency (T(1)/T(p)) of weak scalability for different number of moves and comment the results obtained.

  