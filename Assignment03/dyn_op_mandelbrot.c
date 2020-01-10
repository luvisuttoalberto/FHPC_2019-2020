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

#define n_x_default 8000
#define n_y_default 4000
#define x_L_default -2.75
#define y_L_default 1
#define x_R_default 1.25
#define y_R_default -1
#define I_max_default 255

#define CPU_TIME_W (clock_gettime( CLOCK_REALTIME, &ts ), (double)ts.tv_sec +	\
		    (double)ts.tv_nsec * 1e-9)

void write_pgm_image( void *image, int maxval, int xsize, int ysize, const char *image_name ){

  FILE* image_file;
  
  image_file = fopen(image_name, "w"); 

  int color_depth = 1 + ( (maxval>>8)>0 );
  
  fprintf( image_file, "P5\n%d %d\n%d\n", xsize, ysize, maxval );
  
  fwrite( image, color_depth, xsize * ysize, image_file );  

  fclose( image_file ); 

  return;
}

int main( int argc, char **argv ){

	struct timespec ts;

	int n_x    = n_x_default;
	int n_y    = n_y_default;
	double x_L = x_L_default;
	double y_L = y_L_default;
	double x_R = x_R_default;
	double y_R = y_R_default;
	int I_max  = I_max_default;

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

#pragma omp parallel
	{
#pragma omp master
		{
			nthreads = omp_get_num_threads();
			//printf("omp prefix sum with %d threads \n", nthreads);
		}
	}

	int serial_chunck = iterations/nthreads;
	int chunk_size = serial_chunck/10;



	double delta_x = ( x_R - x_L ) / ( n_x - 1 );
	double delta_y = ( y_L - y_R ) / ( n_y - 1 );
	unsigned char * M = (unsigned char *) calloc( n_x * n_y, sizeof(unsigned char) );

	double tstart  = CPU_TIME_W;

#pragma omp parallel for firstprivate(delta_x, delta_y, x_L, y_L, I_max, n_x, n_y) collapse(2) schedule(dynamic, chunk_size)
	for ( int i = 0; i < n_y; ++i ){
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
				M [i * n_x + j] = k;
			}
		}
	}

	double tend  = CPU_TIME_W;
	printf("process took %g of wall-clock time\n", tend - tstart);
	write_pgm_image( M, I_max, n_x, n_y, "image.pgm" );
	free(M);
	return 0;
}