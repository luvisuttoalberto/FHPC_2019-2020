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
#define USE MPI

#define n_x_default 8000
#define n_y_default 4000
#define x_L_default -2.75
#define y_L_default 1
#define x_R_default 1.25
#define y_R_default -1
#define I_max_default 255

#define CPU_TIME_W (clock_gettime( CLOCK_REALTIME, &ts ), (double)ts.tv_sec +(double)ts.tv_nsec * 1e-9)

int main(int argc, char ** argv){

	struct timespec ts;

	int n_x    = n_x_default;
	int n_y    = n_y_default;
	double x_L = x_L_default;
	double y_L = y_L_default;
	double x_R = x_R_default;
	double y_R = y_R_default;
	int I_max  = I_max_default;
	int numproc;
	int rank;
	int tag_work = 123;
	int tag_finito = 14;
	int tag_libero = 46;



	int iterations = n_x*n_y;
	int nthreads = 1;

	if (( argc > 1 && argc < 8 ) || ( argc > 1 && argc > 8 )){
		printf("arguments should be passed as\n");
		printf("./mandelbrot.x n_x n_y x_L y_L x_R y_R I_max\n");
	}
	else if ( argc > 2 ){ //remember to change this back to 1 when you finish to do the chunck size analysis!!!
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
	
	if(rank == 0){

		int working_process;

		int current_line = 0;

		while(current_line < n_y){

			MPI_Recv(&working_process, 1, MPI_INT, MPI_ANY_SOURCE, tag_libero, MPI_COMM_WORLD, &status);
			MPI_Send(&current_line, 1, MPI_INT, working_process, tag_work, MPI_COMM_WORLD);
			++current_line;
		
		}
		
		for(int i = 1; i < numproc; ++i ){
			MPI_Send(&i, 0, MPI_INT, i, tag_finito, MPI_COMM_WORLD);
		}
	
	}
	else{
		int work_index;
		int offset;
		unsigned char * local_buffer = (unsigned char *) calloc(n_x, sizeof(unsigned char));
		while(1){

			//say to the master that you are free
			MPI_Send(&rank, 1, MPI_INT, 0, tag_libero, MPI_COMM_WORLD);
			
			//wait for the work
			MPI_Recv(&work_index, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			if(status.MPI_TAG == tag_finito){
				printf("Process %d finishing the work\n", rank);
				break;
			}

			for ( int j = 0; j < n_x; ++j ){
				double c_y = y_L - delta_y * work_index;
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
					local_buffer[j] = k;
				}

			}
			//output on file
			offset = work_index * n_x;
			MPI_File_write_at(file, offset, local_buffer, n_x, MPI_INT, &status);
		}
	free(local_buffer);
	}

	MPI_File_close(&file);
	MPI_Finalize();
	return 0;
}