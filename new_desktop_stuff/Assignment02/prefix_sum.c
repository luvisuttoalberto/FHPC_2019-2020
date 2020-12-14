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
	int * mid_sum;
	int * prefix_sum;

	if(argc > 1){
		N = atoi( * (argv+1));
	}
#if defined(_OPENMP)
	if(N%2==0){
		Ntemp = N/2;
		even = 0;
	}
	else{
		Ntemp = (N-1)/2;
		even = 1;
	}
#endif

	if(((array = (int*)malloc(N * sizeof(int))) == NULL) ||
		((mid_sum = (int*)malloc((N/2) * sizeof(int))) == NULL) ||
		((prefix_sum = (int*)malloc(N * sizeof(int))) == NULL)){
		printf("There is not enough memory to host %lu bytes on some thread \n", N*sizeof(int));
		return 1;
	}

#if defined(_OPENMP)
#pragma omp parallel
	{
		int me = omp_get_thread_num();
#pragma omp master
		{
			nthreads = omp_get_num_threads();
			printf("omp prefix sum with %d threads \n", nthreads);
		}
#pragma omp critical
		{
			printf("thread %2d is running on core %2d\n", me, get_cpu_id() );
		}
	}
#else
	printf("serial prefix sum \n");
#endif


//srand48( time(NULL) );
#if defined(_OPENMP)
#pragma omp parallel for
	for ( int ii = 0; ii < N; ii++ ){
    	array[ii] = 1;
    	//array[ii] = drand48();
	}
#else

for ( int ii = 0; ii < N; ii++ ){
    array[ii] = 1;
    //array[ii] = drand48();
}

#endif

	double tstart_1  = CPU_TIME_W;


#if !defined(_OPENMP)
	prefix_sum[0] = array[0];
	for( int i = 1; i<N; ++i){
		prefix_sum[i] = prefix_sum[i-1]+array[i];
	}
#else
#pragma omp parallel for
	for(int i = 0; i<Ntemp; ++i){
		mid_sum[i] = array[i]+array[i+1];
	}	

	double tend_1 = CPU_TIME_W;

//prefix sum seriale del vettore di mezzo
for(int i = 1; i<Ntemp; ++i){
	mid_sum[i] += mid_sum[i-1]; 
}


	double tstart_2  = CPU_TIME_W;
//sposto i valori (che sono definitivi) dal vettore di mezzo al vettore finale
//provo parallelizzando questa parte, controlla che si possa fare (penso di si)
#pragma omp parallel for
	for(int j = 0; j<Ntemp; ++j){
		prefix_sum[j*2+1] = mid_sum[j];
	}
//faccio la somma finale tra prefix e vettore iniziale
if(even == 1){
	++N;
}
prefix_sum[0]=array[0];
#pragma omp parallel for
	for(int ii = 2; ii<N; ii+=2){
		prefix_sum[ii] = prefix_sum[ii-1] + array[ii];
	}

	double tend_2 = CPU_TIME_W;

	printf("process took %g of time\n", tend_1 + tend_2 - tstart_1 - tstart_2);
#endif

	if(even == 1){
		--N;
	}
/*
	for(int jj = 0; jj<N; ++jj){
		printf("prefix_sum[%d] = %d \n", jj, prefix_sum[jj]);
	}
*/
	printf("last number is prefix_sum[%d] = %d \n", N-1, prefix_sum[N-1]);


	free(array);
	free(mid_sum);
	free(prefix_sum);
	return 0;
}







int get_cpu_id( void )
{
#if defined(_GNU_SOURCE)                              // GNU SOURCE ------------
  
  return  sched_getcpu( );

#else

#ifdef SYS_getcpu                                     //     direct sys call ---
  
  int cpuid;
  if ( syscall( SYS_getcpu, &cpuid, NULL, NULL ) == -1 )
    return -1;
  else
    return cpuid;
  
#else      

  unsigned val;
  if ( read_proc__self_stat( CPU_ID_ENTRY_IN_PROCSTAT, &val ) == -1 )
    return -1;

  return (int)val;

#endif                                                // -----------------------
#endif

}



int read_proc__self_stat( int field, int *ret_val )
/*
  Other interesting fields:

  pid      : 0
  father   : 1
  utime    : 13
  cutime   : 14
  nthreads : 18
  rss      : 22
  cpuid    : 39

  read man /proc page for fully detailed infos
 */
{
  // not used, just mnemonic
  // char *table[ 52 ] = { [0]="pid", [1]="father", [13]="utime", [14]="cutime", [18]="nthreads", [22]="rss", [38]="cpuid"};

  *ret_val = 0;

  FILE *file = fopen( "/proc/self/stat", "r" );
  if (file == NULL )
    return -1;

  char   *line = NULL;
  int     ret;
  size_t  len;
  ret = getline( &line, &len, file );
  fclose(file);

  if( ret == -1 )
    return -1;

  char *savetoken = line;
  char *token = strtok_r( line, " ", &savetoken);
  --field;
  do { token = strtok_r( NULL, " ", &savetoken); field--; } while( field );

  *ret_val = atoi(token);

  free(line);

  return 0;
}

