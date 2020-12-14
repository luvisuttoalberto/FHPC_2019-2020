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
	int * prefix_sum;
	int * N_vector;
	int max_lev = 0;



	if(argc > 1){
		N = atoi( * (argv+1));
	}
	
	if(((array = (int*)malloc(N * sizeof(int))) == NULL) || ((prefix_sum = (int*)malloc(N * sizeof(int))) == NULL)){
		printf("There is not enough memory to host %lu bytes on some thread \n", 2*N*sizeof(int));
		return 1;
	}


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
//trovo il numero di livelli
int Ncalcolo = N;
while(Ncalcolo/2 >= nthreads){
	++max_lev;
	Ncalcolo = Ncalcolo/2;
}

//printf("livello max = %d\n", max_lev);

//genero un array di array (contiene gli array intermedi per ogni livello)


if((N_vector = (int*)malloc(max_lev * sizeof(int))) == NULL){
	printf("There is not enough memory to host the array of length %lu \n", max_lev*sizeof(int));
	return 1;
}

//printf("Allocazione N_vector andata a buon fine\n");




//gestisci parità


int ** intermedi;

N_vector[0]=N/2;
//printf("N_vector[0] = %d \n", N_vector[0]);
intermedi = (int**)malloc(max_lev * sizeof(int*));

//printf("Allocazione array esterno andata a buon fine\n");

if((intermedi[0] = (int *)malloc(N_vector[0]*sizeof(int))) == NULL){
	printf("There is not enough memory to host the array of length %lu \n", N_vector[0]*sizeof(int));
	return 1;
}

//printf("Allocazione del primo intermedio andata a buon fine\n");

//controllo che l'allocazione vada a buon fine
for(int i = 1; i<max_lev; ++i){
	N_vector[i] = N_vector[i-1]/2;
	if((intermedi[i] = (int *)malloc(N_vector[i]*sizeof(int))) == NULL){
		printf("There is not enough memory to host the array of length %lu \n", N_vector[i]*sizeof(int));
		return 1;
	}
}
//printf("allocazione a buon fine\n");




//gestisci parità




//srand48( time(NULL) );

#pragma omp parallel for
	for ( int ii = 0; ii < N; ii++ ){
    	array[ii] = 1;
    	//array[ii] = drand48();
	}

//printf("inizializzazione dell'array a buon fine \n");


int curr_lev = 0;

#pragma omp parallel for
for(int i = 0; i<N_vector[curr_lev]; ++i){
	intermedi[curr_lev][i] = array[i*2]+array[i*2+1];
}
++curr_lev;



while(curr_lev < max_lev){
	#pragma omp parallel for
	for(int i = 0; i<N_vector[curr_lev]; ++i){
		intermedi[curr_lev][i] = intermedi[curr_lev-1][i*2]+intermedi[curr_lev-1][i*2+1];
		
	}
	++curr_lev;
}
--curr_lev;

/*for(int i = 0; i< max_lev; i++){
	for (int j = 0; j<N_vector[i]; j++){
		printf("intermedi[%d][%d]=%d \n", i, j, intermedi[i][j]);
	}
}*/


//prefix sum in seriale del livello più interno
for(int i = 1; i<N_vector[max_lev-1]; ++i){
	intermedi[max_lev-1][i] += intermedi[max_lev-1][i-1]; 
	//printf("intermedi[%d][%d]=%d \n", max_lev-1, i, intermedi[max_lev-1][i]);
}



//risalgo nei livelli
//dovrei copiare la roba nei livelli superiori mano a mano?
while(curr_lev>0){
	#pragma omp parallel for
	for(int k = 0; k<N_vector[curr_lev]; ++k){
		intermedi[curr_lev-1][k*2+1] = intermedi[curr_lev][k];
	}

	/*for(int i = 0; i< max_lev; i++){
		for (int j = 0; j<N_vector[i]; j++){
			printf("intermedi[%d][%d]=%d \n", i, j, intermedi[i][j]);
		}
	}*/

	#pragma omp parallel for
	for(int ii = 2; ii<N_vector[curr_lev-1]; ii+=2){
		intermedi[curr_lev-1][ii] += intermedi[curr_lev-1][ii-1];
	}

	/*for(int i = 0; i< max_lev; i++){
		for (int j = 0; j<N_vector[i]; j++){
			printf("intermedi[%d][%d]=%d \n", i, j, intermedi[i][j]);
		}
	}*/


	--curr_lev;
}

#pragma omp parallel for
	for(int k = 0; k<N_vector[curr_lev]; ++k){
		prefix_sum[k*2+1] = intermedi[curr_lev][k];
	}

//ultimo livello
prefix_sum[0] = array[0];
#pragma omp parallel for
	for(int jj = 2; jj < N; jj+=2){
		prefix_sum[jj] = prefix_sum[jj-1] + array[jj];
	}
	
	/*for(int jj = 0; jj<N; ++jj){
		printf("prefix_sum[%d] = %d \n", jj, prefix_sum[jj]);
	}*/
	printf("last number is prefix_sum[%d] = %d \n", N-1, prefix_sum[N-1]);

	for(int i = 0; i<max_lev; ++i){
		free(intermedi[i]);
	}
	free(intermedi);
	free(N_vector);
	free(array);
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

