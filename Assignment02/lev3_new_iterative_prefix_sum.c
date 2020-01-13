#if defined(__STDC__)
#  if (__STDC_VERSION__ >= 199901L)
#     define _XOPEN_SOURCE 700
#  endif
#endif
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sched.h>
#include <omp.h>
#include <math.h>

#define N_default 100
#define max_lev 2

#define CPU_TIME_W (clock_gettime( CLOCK_REALTIME, &ts ), (double)ts.tv_sec +	\
		    (double)ts.tv_nsec * 1e-9)

#define CPU_TIME_T (clock_gettime( CLOCK_THREAD_CPUTIME_ID, &myts ), (double)myts.tv_sec +	\
		     (double)myts.tv_nsec * 1e-9)

#define CPU_TIME_P (clock_gettime( CLOCK_PROCESS_CPUTIME_ID, &ts ), (double)ts.tv_sec +	\
		   (double)ts.tv_nsec * 1e-9)

#define CPU_ID_ENTRY_IN_PROCSTAT 39
#define HOSTNAME_MAX_LENGTH      200

int read_proc__self_stat ( int, int * );
int get_cpu_id           ( void       );

int main(int argc, char ** argv){

	int N = N_default;
	int nthreads = 1;
	int Ntemp;
	int even;

	struct timespec ts;
	int * array;
	int * prefix_sum;
#if defined(_OPENMP)
	int * N_vector;
#endif



	if(argc > 1){
		N = atoi(*(argv+1));
	}
	
	//allocation of the initial array
	if(((array = (int*)malloc(N * sizeof(int))) == NULL) || ((prefix_sum = (int*)malloc(N * sizeof(int))) == NULL)){
		printf("There is not enough memory to host %lu bytes on some thread \n", 2*N*sizeof(int));
		return 1;
	}

#if defined(_OPENMP)
//printing of the number of threads involved in the execution of the program
#pragma omp parallel
	{
		//int me = omp_get_thread_num();
#pragma omp master
		{
			nthreads = omp_get_num_threads();
			printf("omp prefix sum with %d threads \n", nthreads);
		}
	}
#else
		printf("serial prefix sum \n");
		for ( int ii = 0; ii < N; ++ii ){
		    array[ii] = 1;
		}
		
		prefix_sum[0] = array[0];
		for( int i = 1; i<N; ++i){
			prefix_sum[i] = prefix_sum[i-1]+array[i];
		}
		
#endif


#if defined(_OPENMP)
//allocation of the vector that contains the sizes of the intermediate vectors
if((N_vector = (int *)malloc(max_lev * sizeof(int))) == NULL){
	printf("There is not enough memory to host the array of length %lu \n", max_lev * sizeof(int));
	return 1;
}
//declaration of the two dimension array that contains all the intermediate levels used to perform the parallel sums
int ** intermedi;
//allocation of the external array of arrays
if((intermedi = (int **)malloc(max_lev * sizeof(int *))) ==NULL){
	printf("There is not enough memory to host the array of length %lu \n", max_lev * sizeof(int *));
	return 1;
}


/*if((intermedi[0] = (int *)malloc(N_vector[0] * sizeof(int))) == NULL){
	printf("There is not enough memory to host the array of length %lu \n", N_vector[0]*sizeof(int));
	return 1;
}*/

N_vector[0]=N/2;
//allocation of all the internal arrays
for(int i = 0; i<max_lev; ++i){
	if((intermedi[i] = (int *)malloc(N_vector[i]*sizeof(int))) == NULL){
		printf("There is not enough memory to host the array of length %lu \n", N_vector[i]*sizeof(int));
		return 1;
	}
	N_vector[i+1] = N_vector[i]/2;
}

double tstart_1  = CPU_TIME_W; //begin to measure the time of the first parallel section

//initialization in parallel of the initial array
//it is initialized to all ones so that, if the prefix sum works, the last member of the final array is equivalent
//to the number of elements of the initial array
#pragma omp parallel for
	for ( int i = 0; i < N; ++i ){
    	array[i] = 1;
	}

int curr_lev = 0;

//perform the parallel sum of all the couples composing the initial array and write them in the first of the intermediate arrays
//if the size of the array is odd, the last element is taken care of at the reconstruction at the end of the program
#pragma omp parallel for
for(int i = 0; i < N_vector[curr_lev]; ++i){
	intermedi[curr_lev][i] = array[i*2]+array[i*2+1];
}
++curr_lev;

//iterate the previous sum for all the intermediate arrays, until we reach the max level
while(curr_lev < max_lev){
	#pragma omp parallel for
	for(int i = 0; i<N_vector[curr_lev]; ++i){
		intermedi[curr_lev][i] = intermedi[curr_lev-1][i*2]+intermedi[curr_lev-1][i*2+1];
		
	}
	++curr_lev;
}
--curr_lev;

double tend_1  = CPU_TIME_W; //finish to measure the time of the first parallel section

//serial prefix sum of the shortest intermidiate array
for(int i = 1; i<N_vector[curr_lev]; ++i){
	intermedi[curr_lev][i] += intermedi[curr_lev][i-1];
}

double tstart_2  = CPU_TIME_W; //begin to measure the time of the second parallel section

//write the results of the serial prefix sum on the final array
#pragma omp parallel for
for(int i = 0; i<N_vector[max_lev-1]; ++i){
	prefix_sum[i*(int)pow(2, max_lev)+(int)pow(2, max_lev)-1] = intermedi[max_lev-1][i];
}

//go up on all the levels, from the smallest array to the longest one
//performing the sum of each "block" with the performed prefix sum of the shorter array (already written on the final array)
//writing the result directly on the correct position of the final array, iteratively
while(curr_lev>0){

	--curr_lev;

	int i = 0;

	prefix_sum[(i+1)*(int)pow(2, curr_lev+1)-1] = intermedi[curr_lev][i];

	#pragma omp parallel for
	for(i = 2; i<N_vector[curr_lev]; i+=2){
		prefix_sum[(i+1)*(int)pow(2, curr_lev+1)-1] = intermedi[curr_lev][i] + prefix_sum[i*(int)pow(2, curr_lev+1)-1];
	}

}

//use the initial array for the last level
prefix_sum[0] = array[0];
#pragma omp parallel for
	for(int i = 2; i < N; i+=2){
		prefix_sum[i] = prefix_sum[i-1] + array[i];
	}

double tend_2  = CPU_TIME_W; //finish to measure the time of the second parallel section

	printf("process took %g of wall-clock time\n", tend_1 + tend_2 - tstart_1 - tstart_2);
#endif
	printf("last number is prefix_sum[%d] = %d \n", N-1, prefix_sum[N-1]);

	//free the memory allocated
#if defined(_OPENMP)
	for(int i = 0; i<max_lev; ++i){
		free(intermedi[i]);
	}
	free(intermedi);
	free(N_vector);
#endif
	free(array);
	free(prefix_sum);
	return 0;
}