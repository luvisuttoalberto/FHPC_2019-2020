#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define n_x_default 8000
#define n_y_default 4000
#define x_L_default -2.75
#define y_L_default 1
#define x_R_default 1.25
#define y_R_default -1
#define I_max_default 255
#define n_threads_default 6

int write_pgm_image( int maxval, int xsize, int ysize, const char * image_name )
{

  FILE* image_file;

  image_file = fopen(image_name, "w");

  int color_depth = 1 + ( (maxval>>8)>0 );

  int len = fprintf( image_file, "P5\n%d %d\n%d\n", xsize, ysize, maxval );

  fclose( image_file );

  return len;

}

int main( int argc, char **argv )
{

  int n_x    = n_x_default;
  int n_y    = n_y_default;
  double x_L = x_L_default;
  double y_L = y_L_default;
  double x_R = x_R_default;
  double y_R = y_R_default;
  int I_max  = I_max_default;
  int n_threads  = n_threads_default;

  if ( ( argc > 1 && argc < 9 ) || ( argc > 1 && argc > 9 ) )
    {
      printf("arguments should be passed as\n");
      printf("./mandelbrot.x n_x n_y x_L y_L x_R y_R I_max n_threads\n");
    }
  else if ( argc > 1 )
    {
      n_x       = atoi( *(argv + 1) );
      n_y       = atoi( *(argv + 2) );
      x_L       = atof( *(argv + 3) );
      y_L       = atof( *(argv + 4) );
      x_R       = atof( *(argv + 5) );
      y_R       = atof( *(argv + 6) );
      I_max     = atoi( *(argv + 7) );
      n_threads = atoi( *(argv + 8) );
    }

  double delta_x = ( x_R - x_L ) / ( n_x - 1 );
  double delta_y = ( y_L - y_R ) / ( n_y - 1 );

  double c_x;
  double c_y;

  double z_x;
  double z_y;

  double z_x_2;
  double z_y_2;

  int tag_work = 0;
  int tag_stop = 1;
  int tag_free = 2;

  MPI_File file;
  MPI_Status status;

  MPI_Init( &argc, &argv );

  int rank;
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );

  int size;
  MPI_Comm_size( MPI_COMM_WORLD, &size );

  MPI_File_open( MPI_COMM_WORLD, "image_hybrid.pgm", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO\
_NULL, &file );

  double tstart = MPI_Wtime();

  int header;

  if( rank == 0 ) header = write_pgm_image( I_max, n_x, n_y, "image_hybrid.pgm" );

  MPI_Bcast( &header, 1, MPI_INT, 0, MPI_COMM_WORLD );

  if ( rank == 0 )
    {

      int process;

      int line [2];


      line [0] = 0;
      line [1] = n_threads;

      while( line [0] + n_threads < n_y )
        {

          MPI_Recv( &process, 1, MPI_INT, MPI_ANY_SOURCE, tag_free, MPI_COMM_WORLD, &status );

          MPI_Send( &line, 2, MPI_INT, process, tag_work, MPI_COMM_WORLD );

          line [0] += n_threads;

        }

      int rem = n_y - 1 - line [0];

      if ( rem != 0 )
        {

          line [1] = rem;

          MPI_Recv( &process, 1, MPI_INT, MPI_ANY_SOURCE, tag_free, MPI_COMM_WORLD, &status );

          MPI_Send( &line, 2, MPI_INT, process, tag_work, MPI_COMM_WORLD );

        }

      for ( int i = 1; i < size; ++i )
 {

          MPI_Recv( &process, 1, MPI_INT, MPI_ANY_SOURCE, tag_free, MPI_COMM_WORLD, &status );

          MPI_Send( &i, 0, MPI_INT, process, tag_stop, MPI_COMM_WORLD );

 }

    }
  else
    {

      int index [2];

      int len = n_x * n_threads;

      unsigned char * local = (unsigned char *) malloc( len * sizeof(unsigned char) );

        while( 1 )
          {

            MPI_Send( &rank, 1, MPI_INT, 0, tag_free, MPI_COMM_WORLD );

            MPI_Recv( &index, 2, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status ) ;

            if( status.MPI_TAG == tag_stop ) break;

            if( index [1] != n_threads )
              {

                len = n_x * index [1];

                local = (unsigned char *) realloc( local, len * sizeof(unsigned char) );

              }

            #pragma omp parallel for collapse(2) schedule(guided)

            for ( int i = index [0]; i < index [0] + index [1]; ++i )
              {

                for ( int j = 0; j < n_x; ++j )
                  {

                    c_y = y_L - delta_y * i;
                    c_x = x_L + delta_x * j;

                    z_x = 0;
                    z_y = 0;

                    z_x_2 = z_x * z_x;
                    z_y_2 = z_y * z_y;

                    int k;


for ( k = 0; k < I_max; ++k )
                      {

                        if ( z_x_2 + z_y_2 > 4 )
                          {
                            local [(i - index [0]) * n_x + j] = k;

                            break;
                          }

                        z_y = 2 * z_x * z_y + c_y;
                        z_x = z_x_2 - z_y_2 + c_x;

                        z_x_2 = z_x * z_x;
                        z_y_2 = z_y * z_y;

                      }

                    if ( k == I_max ) local [(i - index[0]) * n_x + j] = 0;


                  }

              }

            int offset = header + index [0] * n_x;

            MPI_File_write_at( file, offset, local, len, MPI_UNSIGNED_CHAR, &status );

          }

      free( local );

    }

  double tend = MPI_Wtime();

  printf("process took %g of wall-clock time\n", tend - tstart);

  MPI_File_close( &file );

  MPI_Finalize();

  return 0;
}