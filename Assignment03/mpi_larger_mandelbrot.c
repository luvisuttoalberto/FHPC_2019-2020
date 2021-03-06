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
#include <math.h>
#include <mpi.h>
#include <omp.h>
#define USE MPI

#define n_x_default 8000
#define n_y_default 4000
#define x_L_default -2.75
#define y_L_default 1
#define x_R_default 1.25
#define y_R_default -1
#define I_max_default 255
#define step_default 10

//#define CPU_TIME_W (clock_gettime( CLOCK_REALTIME, &ts ), (double)ts.tv_sec +(double)ts.tv_nsec * 1e-9)

int write_pgm_image(int maxval, int xsize, int ysize, const char *image_name ){

  FILE* image_file;
  
  image_file = fopen(image_name, "w"); 

  int color_depth = 1 + ( (maxval>>8)>0 );
  
  int len = fprintf( image_file, "P5\n%d %d\n%d\n", xsize, ysize, maxval );

  fclose( image_file ); 

  return len;
}


/*
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
 *//*
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


*/

int main(int argc, char ** argv){

	//struct timespec ts;

	int n_x    = n_x_default;
	int n_y    = n_y_default;
	double x_L = x_L_default;
	double y_L = y_L_default;
	double x_R = x_R_default;
	double y_R = y_R_default;
	int I_max  = I_max_default;
	int step = step_default;
	int numproc;
	int rank;
	int header_len;
	int tag_work = 123;
	int tag_finito = 14;
	int tag_libero = 46;



	int iterations = n_x*n_y;

	if (( argc > 1 && argc < 8 ) || ( argc > 1 && argc > 8 )){
		printf("arguments should be passed as\n");
		printf("./mandelbrot.x n_x n_y x_L y_L x_R y_R I_max\n");
	}
	else if ( argc > 1 ){
		n_x   = atoi( *(argv + 1) );
		n_y   = atoi( *(argv + 2) );
		x_L   = atof( *(argv + 3) );
		y_L   = atof( *(argv + 4) );
		x_R   = atof( *(argv + 5) );
		y_R   = atof( *(argv + 6) );
		I_max = atoi( *(argv + 7) );
	}

	double delta_x = ( x_R - x_L ) / ( n_x - 1 );
	double delta_y = ( y_L - y_R ) / ( n_y - 1 );


	MPI_File file;
	MPI_Status status;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_File_open(MPI_COMM_WORLD, "image.pgm", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &file);

	double tstart = MPI_Wtime();

	if(rank == 0){
		header_len = write_pgm_image(I_max, n_x, n_y, "image.pgm");
	}
	MPI_Bcast(&header_len, 1, MPI_INT, 0, MPI_COMM_WORLD);


	if(rank == 0){

		int working_process;
		int current_line[2];
		current_line[0] = 0;
		current_line[1] = step;

		while(current_line[0] + step < n_y){

			MPI_Recv(&working_process, 1, MPI_INT, MPI_ANY_SOURCE, tag_libero, MPI_COMM_WORLD, &status);
			MPI_Send(&current_line, 2, MPI_INT, working_process, tag_work, MPI_COMM_WORLD);
			current_line[0] += step;
		}

		int remainder = n_y - 1 - current_line[0];
		if(remainder != 0){
			current_line[1] = remainder;
			MPI_Recv(&working_process, 1, MPI_INT, MPI_ANY_SOURCE, tag_libero, MPI_COMM_WORLD, &status);
			MPI_Send(&current_line, 2, MPI_INT, working_process, tag_work, MPI_COMM_WORLD);
		}

		for(int i = 1; i < numproc; ++i ){
			MPI_Recv(&working_process, 1, MPI_INT, MPI_ANY_SOURCE, tag_libero, MPI_COMM_WORLD, &status);
			MPI_Send(&i, 0, MPI_INT, working_process, tag_finito, MPI_COMM_WORLD);
		}
	}
	else{
		int work_index[2];
		int offset;
		int stop;
		int len = step * n_x;
		unsigned char * local_buffer = (unsigned char *) malloc(len * sizeof(unsigned char));
		while(1){

			//say to the master that you are free
			MPI_Send(&rank, 1, MPI_INT, 0, tag_libero, MPI_COMM_WORLD);
			
			//wait for the work
			MPI_Recv(&work_index, 2, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			if(status.MPI_TAG == tag_finito){
				printf("Process %d finishing the work\n", rank);
				break;
			}
			if(work_index[1] != step){
				len = n_x * work_index[1];
				local_buffer = (unsigned char *) realloc(local_buffer, len * sizeof(unsigned char));
			}
			stop = work_index[0] + work_index[1];
			/*#pragma omp parallel
			{
				int me = omp_get_thread_num();
				#pragma omp master
				{
					step = omp_get_num_threads();
					printf("omp prefix sum with %d threads \n", step);
				}
				#pragma omp critical
				{
					printf("thread %2d spawned by process %2d is running on core %2d\n", me, rank, get_cpu_id() );
				}
			}*/


			//#pragma omp parallel for firstprivate(delta_x, delta_y, x_L, y_L, I_max, n_x, n_y) collapse(2) schedule(guided)
			for(int i = work_index[0]; i < stop; ++i){
				for ( int j = 0; j < n_x; ++j ){
					double c_y = y_L - delta_y * i;
					double c_x = x_L + delta_x * j;
					double z_x = 0;
					double z_y = 0;
					double z_x_2 = z_x * z_x;
					double z_y_2 = z_y * z_y;
					int k = 0;
					
					while(k < I_max && z_x_2 + z_y_2 < 4){
						z_y = 2 * z_x * z_y + c_y;
						z_x = z_x_2 - z_y_2 + c_x;
						z_x_2 = z_x * z_x;
						z_y_2 = z_y * z_y;
						++k;
					}

					if(k!=I_max){
						local_buffer[(i-work_index[0]) * n_x + j] = k;
					}
					else{
						local_buffer[(i-work_index[0]) * n_x + j] = 255;
					}

				}
			}
			//output on file
			offset = header_len + work_index[0] * n_x;
			MPI_File_write_at(file, offset, local_buffer, len, MPI_UNSIGNED_CHAR, &status);
		}
	free(local_buffer);
	}

	double tend = MPI_Wtime();
	printf("process took %g of wall-clock time\n", tend - tstart);

	MPI_File_close(&file);
	MPI_Finalize();
	return 0;
}