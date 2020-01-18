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

//function used to write the header on the file, returns the length of the written string
int write_header(int maxval, int xsize, int ysize, const char *image_name ){

  FILE* image_file;
  
  image_file = fopen(image_name, "w"); 

  int color_depth = 1 + ( (maxval>>8)>0 );
  
  int len = fprintf( image_file, "P5\n%d %d\n%d\n", xsize, ysize, maxval );

  fclose( image_file ); 

  return len;
}


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
	int header_len;
	int tag_work = 123;
	int tag_end = 14;
	int tag_free = 46;

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

	//calculate the resolution of the matrix
	double delta_x = ( x_R - x_L ) / ( n_x - 1 );
	double delta_y = ( y_L - y_R ) / ( n_y - 1 );

	MPI_File file;
	MPI_Status status;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_File_open(MPI_COMM_WORLD, "image.pgm", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &file);
	
	double tstart = MPI_Wtime(); //begin to measure the computation time

	//only the master writes the header on the file
	if(rank == 0){
		header_len = write_header(I_max, n_x, n_y, "image.pgm");
	}
	//the master communicates the length of the header to all the other processes
	//in this way every process can calculate on his own the correct offset when he will need to write on the file 
	MPI_Bcast(&header_len, 1, MPI_INT, 0, MPI_COMM_WORLD);


	if(rank == 0){//master process part

		int working_process;

		int current_line = 0;

		//begin to distribute the work
		while(current_line < n_y){

			//the master waits until a process tells him that he is free
			MPI_Recv(&working_process, 1, MPI_INT, MPI_ANY_SOURCE, tag_free, MPI_COMM_WORLD, &status);
			//then he sends to the free process the line on which he has to work
			MPI_Send(&current_line, 1, MPI_INT, working_process, tag_work, MPI_COMM_WORLD);
			++current_line;
		}
		
		//the work is finished
		for(int i = 1; i < numproc; ++i ){
			//the master wait until a process tells him that he is free
			MPI_Recv(&working_process, 1, MPI_INT, MPI_ANY_SOURCE, tag_free, MPI_COMM_WORLD, &status);
			//the master sends to the free process an empty message saying that the work is finished (tag_free)
			MPI_Send(&i, 0, MPI_INT, working_process, tag_end, MPI_COMM_WORLD);
		}
	}
	else{//slave processes part
		int work_index;
		int offset;
		//every process allocate a local buffer in which he puts his partial results before writing them in the final image 
		unsigned char * local_buffer = (unsigned char *) malloc(n_x * sizeof(unsigned char));
		while(1){

			//the process says to the master that he is free
			MPI_Send(&rank, 1, MPI_INT, 0, tag_free, MPI_COMM_WORLD);
			
			//the process wait to receive the work from the master
			MPI_Recv(&work_index, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

			if(status.MPI_TAG == tag_end){ //if the message received is with the finish tag (tag_end)
				printf("Process %d finishing the work\n", rank); //the process prints that he is finishing
				break; //and break out of the while
			}
			//otherwise, if the message contains work, the slave performs the work
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
				else{
					local_buffer[j] = 0;
				}

			}
			//the slave writes the result on the file
			offset = header_len + work_index * n_x;
			MPI_File_write_at(file, offset, local_buffer, n_x, MPI_UNSIGNED_CHAR, &status);
		}
	free(local_buffer); //free of the allocated local buffer by every thread
	}

	double tend = MPI_Wtime();//finish to measure the computation time
	printf("process took %g of wall-clock time\n", tend - tstart);
	MPI_File_close(&file);
	MPI_Finalize();
	return 0;
}
