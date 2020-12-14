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
	int * N_vector;
	int max_lev = 0;

	double ** time_start;
	double ** time_end;

	if((time_start = (double **)malloc(max_lev * sizeof(double *))) ==NULL){
		printf("There is not enough memory to host the array of length %lu \n", 2 * sizeof(double *));
		return 1;
	}

	if((time_end = (double **)malloc(max_lev * sizeof(double *))) ==NULL){
		printf("There is not enough memory to host the array of length %lu \n", 2 * sizeof(double *));
		return 1;
	}


	if(argc > 1){
		N = atoi(*(argv+1));
	}
	
	if(((array = (int*)malloc(N * sizeof(int))) == NULL) || ((prefix_sum = (int*)malloc(N * sizeof(int))) == NULL)){
		printf("There is not enough memory to host %lu bytes on some thread \n", 2*N*sizeof(int));
		return 1;
	}


#pragma omp parallel
	{
		//int me = omp_get_thread_num();
#pragma omp master
		{
			nthreads = omp_get_num_threads();
			printf("omp prefix sum with %d threads \n", nthreads);
		}
	}

int Ncalcolo = N;
while(Ncalcolo/2 >= nthreads){
	++max_lev;
	Ncalcolo = Ncalcolo/2;
}

for(int i = 0; i<2; ++i){
	if((time_start[i] = (double *)malloc(max_lev*sizeof(double))) == NULL){
		printf("There is not enough memory to host the array of length %lu \n", N_vector[i]*sizeof(double));
		return 1;
	}
}

for(int i = 0; i<2; ++i){
	if((time_end[i] = (double *)malloc(max_lev*sizeof(double))) == NULL){
		printf("There is not enough memory to host the array of length %lu \n", N_vector[i]*sizeof(double));
		return 1;
	}
}



if((N_vector = (int *)malloc(max_lev * sizeof(int))) == NULL){
	printf("There is not enough memory to host the array of length %lu \n", max_lev * sizeof(int));
	return 1;
}

int ** intermedi;

N_vector[0]=N/2;

if((intermedi = (int **)malloc(max_lev * sizeof(int *))) ==NULL){
	printf("There is not enough memory to host the array of length %lu \n", max_lev * sizeof(int *));
	return 1;
}


if((intermedi[0] = (int *)malloc(N_vector[0] * sizeof(int))) == NULL){
	printf("There is not enough memory to host the array of length %lu \n", N_vector[0]*sizeof(int));
	return 1;
}

for(int i = 1; i<max_lev; ++i){
	N_vector[i] = N_vector[i-1]/2;
	if((intermedi[i] = (int *)malloc(N_vector[i]*sizeof(int))) == NULL){
		printf("There is not enough memory to host the array of length %lu \n", N_vector[i]*sizeof(int));
		return 1;
	}
}

double tstart_1  = CPU_TIME_W;

#pragma omp parallel for
	for ( int i = 0; i < N; ++i ){
    	array[i] = 1;
	}

int curr_lev = 0;

#pragma omp parallel for
for(int i = 0; i < N_vector[curr_lev]; ++i){
	intermedi[curr_lev][i] = array[i*2]+array[i*2+1];
}
++curr_lev;

while(curr_lev < max_lev){
	time_start[0][curr_lev] = CPU_TIME_W;
	#pragma omp parallel for
	for(int i = 0; i<N_vector[curr_lev]; ++i){
		intermedi[curr_lev][i] = intermedi[curr_lev-1][i*2]+intermedi[curr_lev-1][i*2+1];
		
	}
	time_end[0][curr_lev] = CPU_TIME_W;
	++curr_lev;
}
--curr_lev;

double tend_1  = CPU_TIME_W;

//prefix sum in seriale del livello più interno
for(int i = 1; i<N_vector[curr_lev]; ++i){
	intermedi[curr_lev][i] += intermedi[curr_lev][i-1];
}

double tstart_2  = CPU_TIME_W;

#pragma omp parallel for
for(int i = 0; i<N_vector[max_lev-1]; ++i){
	prefix_sum[i*(int)pow(2, max_lev)+(int)pow(2, max_lev)-1] = intermedi[max_lev-1][i];
}


while(curr_lev>0){

	--curr_lev;

	int i = 0;

	time_start[1][curr_lev+1] = CPU_TIME_W;

	prefix_sum[(i+1)*(int)pow(2, curr_lev+1)-1] = intermedi[curr_lev][i];

	#pragma omp parallel for
	for(i = 2; i<N_vector[curr_lev]; i+=2){
		prefix_sum[(i+1)*(int)pow(2, curr_lev+1)-1] = intermedi[curr_lev][i] + prefix_sum[i*(int)pow(2, curr_lev+1)-1];
	}

	time_end[1][curr_lev+1] = CPU_TIME_W;

}

//ultimo livello
prefix_sum[0] = array[0];
#pragma omp parallel for
	for(int i = 2; i < N; i+=2){
		prefix_sum[i] = prefix_sum[i-1] + array[i];
	}

double tend_2  = CPU_TIME_W;

	//printf("process took %g of wall-clock time\n", tend_1 + tend_2 - tstart_1 - tstart_2);

	//printf("last number is prefix_sum[%d] = %d \n", N-1, prefix_sum[N-1]);
	
	for(int i = 1; i<max_lev; ++i){
		printf("time level %d = %g \n", i, time_end[1][i] + time_end[0][i] - time_start[1][i] - time_start[0][i]);
	}

	for(int i = 0; i<max_lev; ++i){
		free(intermedi[i]);
	}
	for(int i = 0; i<2; ++i){
		free(time_start[i]);
	}
	for(int i = 0; i<2; ++i){
		free(time_end[i]);
	}
	free(time_start);
	free(time_end);
	free(intermedi);
	free(N_vector);
	free(array);
	free(prefix_sum);
	return 0;
}