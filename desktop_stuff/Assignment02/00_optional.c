#if defined(__STDC__)
#  if (__STDC_VERSION__ >= 199901L)
#     define _XOPEN_SOURCE 700
#  endif
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <omp.h>


#define N_default 100

#if defined(_OPENMP)
#define CPU_TIME (clock_gettime( CLOCK_REALTIME, &ts ), (double)ts.tv_sec + \
		  (double)ts.tv_nsec * 1e-9)

#define CPU_TIME_th (clock_gettime( CLOCK_THREAD_CPUTIME_ID, &myts ), (double)myts.tv_sec + \
		     (double)myts.tv_nsec * 1e-9)
#else

#define CPU_TIME (clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts ), (double)ts.tv_sec + \
		   (double)ts.tv_nsec * 1e-9)

#endif





int main( int argc, char **argv )
{

  int     N        = N_default;
  int     nthreads = 1;
  
  struct  timespec ts;
  double *array;
  double S           = 0;                                   // this will store the summation



  /*  -----------------------------------------------------------------------------
   *   initialize 
   *  -----------------------------------------------------------------------------
   */

  // check whether some arg has been passed on
  if ( argc > 1 )
    N = atoi( *(argv+1) );


  // allocate memory
  if ( (array = (double*)malloc( N * sizeof(double) )) == NULL )
    {
      printf("I'm sorry, there is not enough memory to host %lu bytes\n", N * sizeof(double) );
      return 1;
    }

  // just give notice of what will happen and get the number of threads used
#ifndef _OPENMP
  printf("serial summation\n");
  for ( int ii = 0; ii < N; ii++ )
    array[ii] = (double)ii;      
#else
#pragma omp parallel
  {
    int me = omp_get_thread_num();
#pragma omp master
    {
      nthreads = omp_get_num_threads();
      printf("omp summation with %d threads\n", nthreads );
    }

    
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
  // initialize the array
  //srand48( time(NULL) );
  for ( int ii = start; ii < stop; ++ii ){
    array[ii] = (double)ii;                                 // choose the initialization you prefer;
    //printf("array[%d] = %g \n", ii, array[ii]);
  }
    
    //array[ii] = drand48();                                // the first one (with integers) makes it
                                                            // to check the result
#pragma omp parallel reduction(+:S){
    for ( int ii = start; ii < stop; ii++ ){
      S += array[ii];
    }

}                     

}
#endif
/*for(int i = 0; i<N; ++i){
  printf("array[%d] = %g \n", i, array[i]);
}*/
  /*  -----------------------------------------------------------------------------
   *   calculate
   *  -----------------------------------------------------------------------------
   */


  /*double th_avg_time = 0;                                   // this will be the average thread runtime
  double th_min_time = 1e11;                                   // this will be the min thread runtime.
							    // contrasting the average and the min
							    // time taken by the threads, you may
							    // have an idea of the unbalance.

  double tstart  = CPU_TIME;*/
  
#if !defined(_OPENMP)
  
  for ( int ii = 0; ii < N; ii++ )                          // well, you may notice this implementation
    S += array[ii];                                         // is particularly inefficient anyway
  

#endif

  //double tend = CPU_TIME;                                   // this timer is CLOCK_REALTIME if OpenMP
							    // is active; CLOCK_PROCESS_CPU_TIME_ID
							    // otherwise. That is because the latter
							    // would accounts for the whole cpu time
							    // used by the threads under OpenMP.

  /*  -----------------------------------------------------------------------------
   *   finalize
   *  -----------------------------------------------------------------------------
   */

printf("Sum is %g, process took %g of wall-clock time\n\n"
       "<%g> sec of avg thread-time\n"
       "<%g> sec of min thread-time\n",
       S, tend - tstart, th_avg_time/nthreads, th_min_time );
  
  free( array );
  return 0;
}
