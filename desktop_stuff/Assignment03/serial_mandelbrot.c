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
	unsigned char * M = (unsigned char *) calloc( n_x * n_y, sizeof(unsigned char) );

	double c_x;
	double c_y;
	double z_x;
	double z_y;
	double z_x_2;
	double z_y_2;

	double tstart  = CPU_TIME_W;

	for ( int i = 0; i < n_y; ++i ){
		c_y = y_L - delta_y * i;
		for ( int j = 0; j < n_x; ++j ){
			c_x = x_L + delta_x * j;
			z_x = 0;
			z_y = 0;
			z_x_2 = z_x * z_x;
			z_y_2 = z_y * z_y;
			for ( int k = 0; k < I_max; ++k ){
				if ( z_x_2 + z_y_2 > 4 ){
					M [i * n_x + j] = k;
					break;
				}
				else{
					M [i * n_x + j] = 0;
				}
				z_y = 2 * z_x * z_y + c_y;
				z_x = z_x_2 - z_y_2 + c_x;
				z_x_2 = z_x * z_x;
				z_y_2 = z_y * z_y;
			}
		}
	}

	double tend  = CPU_TIME_W;

	printf("process took %g of wall-clock time\n", tend - tstart);	

	write_pgm_image( M, I_max, n_x, n_y, "image.pgm" );
	free(M);
	return 0;
}