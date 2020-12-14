# Second Assignment							

Luvisutto Alberto

## Exercise 0

### Touch-first vs Touch-by-All policy

#### 1. Measure the time to solution of the two codes in a strong scaling test

I compiled the two codes with the latest version of OpenMP available on the Ulysses cluster. 

In the following graph are reported the two speedups, for the two different versions of the code. The perfectly linear speedup is also inserted in the graph just to give an idea of the ideal speedup.

INSERT IMAGE SPEEDUP

It can be observed that, while the Touch-by-All policy code scales a bit, the Touch-first one doesn't scale at all. The reason will be analyzed in the third point of this exercise.

#### 2. Measure the parallel overhead of both codes

It is clear, from the following graph, that having the array initialized separately for every thread, and the values of the array in each thread's own cache, lowers a lot the parallel overhead in the Touch-by-All policy code. This is also the reason behind the speedup boost on the second code.

INSERT IMAGE PARALLEL OVERHEAD

It can also be observed an oscillation in the overhead of the Touch-first policy code. This could be due to the fact that the threads (and cores) are distributed in the same node, but on two different sockets. This distance between sockets can corresponds to a higher time in accessing the memory of the thread 0, that in this case is the one who initializes the array.

#### 3. Provide any relevant metrics that explain any observed difference

I used perf to profile the two codes and the followings are the key results that I obtained. I mainly focused on cache-misses and instructions per cycle (clearly influenced by the cache misses but still an important index of the good/bad design of the code).

With the use of a shell script I took some data with different numbers of threads both for the first and second program and tried to plot the results in the following graphs.

INSERT IMAGE CACHE MISSES

It is clear from the previous graph that, independently from the number of threads involved, the number of cache misses in the Touch-first code is a lot higher than in the second one. For the reason mentioned before, this is exactly the expected behavior of the programs; the fact that every thread initializes his own part of the array (in the Touch-by-All policy) lowers a lot the number of times in which every thread doesn't find the desired value in his own cache.

#### 4. Optional: figure out how to allocate and correctly initialize the right amount of memory separately on each thread

Since the `#pragma omp parallel for` used in the Touch-by-All policy code already initialize the right amount of memory separately on each thread, I tried to implement a code without using the `parallel for` command, but only normal parallel regions. For this reason, I calculated at the beginning of the program the size of the section each thread had to initialize, dividing the initial array in smaller and mostly equal pieces (dealing with the possible remainder). In the code that illustrate this passage, successively reported, it is also added a `#pragma omp barrier`; in this way I avoid to initialize the variables `start`, `stop` and `dist` before having the real number of threads involved in the execution ready for the computation (since this value is computed only by the master). 

```c
int nthreads;
#pragma omp parallel{
	int me = omp_get_thread_num();
	#pragma omp master{
		nthreads = omp_get_num_threads();
      	printf("omp summation with %d threads\n", nthreads );
	}

	#pragma omp barrier
	
	int start, stop, dist;
  	int r = N%nthreads;
  	if(r == 0){
      dist = N/nthreads;
      start = me*dist;
      stop = start + dist;
  	}else{
      if(me < r){
          dist = N/nthreads + 1;
          start = me*dist;
          stop = start + dist;
      }
      else{
          dist = N/nthreads;
          start = r * (dist + 1) + (me - r) * dist;
          stop = start + dist;
      }
  	}
	for ( int ii = start; ii < stop; ++ii ){
		array[ii] = (double)ii;
	}
}
```

The rest of the code would be implemented by other normal parallel regions, used to divide equally the computation between all the threads (similarly to the code reported before), collecting then the final local results. The performance of the final code will probably be worse than the Touch-by-All policy code, because I simply tried to do explicitly (and with only a few lines of code) a very complex process that is dealt in a much more complicated and optimized way by the `#pragma omp parallel for` mentioned before.

## Exercise 3

### Prefix sum with OpenMP

To avoid confusion, the only version of the code in the directory is the final one. Nevertheless, I will go through the whole process that brought me to the final implementation of the code. Given that the differences between the implementations are small (and hopefully well explained in this report) , I kept a copy of the intermediate versions; if required, I can send them for a better understanding of the process. The final code is also commented, to better explain each passage of the program. I will now give a brief explanation of the algorithm I used.

#### Explanation of the algorithm

I tried to implement this code following a "divide et impera" philosophy. Given that a serial prefix summation must be done at some point in the code, I tried to do it on the smallest possible array. 

For this reason, I implemented at first a code that, starting from the initial array, perform the parallel sum of each couple of elements, writing the result on an array of half size.

I then iterated this part until I got to an array "small enough" (= with size smaller than the number of threads involved in the computation). In the following image it can be seen an example of execution with 2 threads.

INSERT IMAGE EXAMPLE

Then a serial prefix sum is performed on the smallest array. In this way I tried to keep the serial part of the code as small as possible.

Then, iteratively, I go back through all the intermediate arrays, from the smallest to the biggest, performing the parallel sum between the final values, already written on the final array, and the remaining ones on the intermediate vectors, writing the final sum in the correspondent place of the final array. It is provided an example:

INSERT IMAGE EXAMPLE

The possible odd sizes of the intermediate arrays are automatically dealt with the choice of the indexes in the for loops of this part.

#### Results and comparison

I took the times on the Ulysses cluster using a bash script and obtained the following graphs. All the graphs are obtained (if not specifically mentioned) using the elapsed time, taken with the `/usr/bin/time` command.

INSERT IMAGE SPEEDUP

INSERT IMAGE SERIAL FRACTION

INSERT IMAGE PARALLEL OVERHEAD

Clearly, there was something wrong with the code; it wasn't scaling at all, there was too much overhead and it was also increasing a lot with the number of threads.

For this reason, I tried to take the time that took to perform the parallel summation of the couples of elements, for each level, for different numbers of threads. The results can be seen in the following graph.

INSERT IMAGE PICCO A 20

It is clear that, at some point, the parallel computation requires too much time; it could be not worth it to avoid serial summation to perform this parallel sum in this amount of time.

For this reason, I thought that maybe going down to the smallest possible level would just increase the total time required by the program, with no benefit in the scalability. I then proceeded to collect, with the following bash script, the total time of execution of the program varying both the number of threads and the maximum level at which the program should stop. I obtained the following graph.

INSERT IMAGE LIVELLO 2 IS THE BEST

It can be observed that there is a minimum on the times when the program stops the iteration at the second level, when the number of threads involved is bigger than 2. This suggests that, probably, this number increases with the number of threads involved (of course depending also on the size of the problem), although this couldn't be proved, because the computation should have been done multiple times with a very larger number of threads (and so with a very large number of cores). I did it up to 60 cores (so 60 threads), but the most convenient level was still the second.

Unable to obtain, for the reported reasons, an experimental formula to calculate the maximum level at which the program should stop, I run again the program inserting a "magic number" (`max_lev = 2`) in the code, to make the iteration stop at the "right" level. This is the final version of the code I implemented.

In the following graphs it can be observed the difference between this version of the parallel code and the previous one, where the iteration was done on every possible level.

INSERT IMAGE SPEEDUP COMPARISON

From the previous graph we can observe that the two speedups are very different, even if they both get quite flat very soon. This flatness may be due to the fact that this algorithm should function a lot better with a very larger number of threads.

INSERT IMAGE PARALLEL OVERHEAD COMPARISON

From the previous graph, it can be observed that the parallel overhead is reduced by a factor of two in the final version, even if it's still increasing. This is due to the fact that there are a lot less "waiting" moments in which every thread try to access the memory in the deepest levels.

INSERT IMAGE SERIAL FRACTION COMPARISON

From this previous graph we can also see that despite the fact that the serial part is increased from the previous to the final version, the serial fraction is less than before, because the parallel overhead is highly reduced.

We can also show, in the following graph, the speedup comparison based on the wallclock times taken inside the program, regarding only the parallelizable parts of the code.

INSERT IMAGE COMPARISON WALLCLOCK TIME SPEEDUP

It can be observed that even considering the wallclock time, the speedup is increased with respect to the previous version of the code.