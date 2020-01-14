#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <mpi.h>
#define USE MPI
#define SEED 35791246

#define Nx_default 8000
#define Ny_default 4000
#define X_l_default -2.75
#define Y_l_default 1
#define X_r_default 1.25
#define Y_r_default -1
#define Imax_default 255

int mandelbrot( int , double , double ,double ,double );
int start_image( int , int , int , const char *  );
int main ( int argc , char *argv[ ] )
{ int Nx    = Nx_default;
  int Ny    = Ny_default;
  double X_l = X_l_default;
  double Y_l = Y_l_default;
  double X_r = X_r_default;
  double Y_r = Y_r_default;
  int Imax  = Imax_default;

  if ( ( argc > 1 && argc < 8 ) || ( argc > 1 && argc > 8 ) )
    {
      printf("arguments should be passed as\n");
      printf("./mandelbrot.x n_x n_y x_L y_L x_R y_R I_max n_threads\n");
    }
  else if ( argc > 1 )
    {
      Nx        = atoi( *(argv + 1) );
      Ny        = atoi( *(argv + 2) );
      X_l       = atof( *(argv + 3) );
      Y_l       = atof( *(argv + 4) );
      X_r       = atof( *(argv + 5) );
      Y_r       = atof( *(argv + 6) );
      Imax      = atoi( *(argv + 7) );

    }

                                                                                                                                      
  double start_time, end_time; // times 
  int myid , numprocs , proc ;
  MPI_File file;
  MPI_Status status;
  MPI_Request request;                                                                                                                          
  int master = 0;// master process 
  int tag = 123;
  MPI_Init(&argc,&argv);//start MPI
  MPI_Comm_size(MPI_COMM_WORLD,&numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD,&myid);
  double delta_x = (X_r-X_l)/Nx;
  double delta_y = (Y_r-Y_l)/Ny;

  //read N from line
  start_time = MPI_Wtime();

  int header;
  if( myid == 0 ) header = start_image( Imax, Nx, Ny, "image.pgm" );
  MPI_Bcast( &header, 1, MPI_INT, 0, MPI_COMM_WORLD );
  if (myid ==0) 
    { 
      int name;
      int i=0;                                                            
      while (i<Ny)
	{ 
	  int index=i*Nx;
	  MPI_Recv( &name, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status );
	  MPI_Send( &index, 2, MPI_INT, name, 1, MPI_COMM_WORLD );
	  i++;
	  printf("%d \n",i);
	}
      for ( int i = 1; i < numprocs; ++i )
	{

          MPI_Recv( &myid, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status );

          MPI_Send( &i, 0, MPI_INT, myid, 2, MPI_COMM_WORLD );

	}
    }
  else 
    { unsigned char * proc_arr = (unsigned char *) malloc( sizeof(unsigned char)* Nx );
      while (1)
	{
      int index;
      MPI_Send( &myid, 1, MPI_INT, 0, 0, MPI_COMM_WORLD );
      MPI_Recv( &index, 2, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status ) ;
      if( status.MPI_TAG == 2 ) break;
      for(unsigned int ii=0; ii<Nx; ii++)
	{ 
	  int i=index/Nx;
	  double c_im = Y_l + i*delta_y;
	  double c_re = X_l+ii*delta_x; 
	  double Z_re=0,Z_im=0;
	  proc_arr[ii]=mandelbrot(Imax, Z_re, Z_im,c_im,c_re);
	}
      int offset=header+index;
      MPI_File_write_at( file, offset, proc_arr, Nx, MPI_UNSIGNED_CHAR, &status );
	}
      free( proc_arr );
    }
  end_time=MPI_Wtime();
  printf ( "\n # walltime : %10.8f \n", end_time - start_time) ;

  MPI_File_close( &file );  
  MPI_Finalize() ;
  return 0; // let MPI finish up /                                                                                                     
}

int mandelbrot( int Imax, double Z_re, double Z_im,double c_im,double c_re)
{ 
  unsigned char colorind=0;
  for(unsigned char n=0; n<Imax+1; n++)
    { 
      colorind=0;
      double Z_reS, Z_imS;  
      Z_reS = Z_re*Z_re;
      Z_imS = Z_im*Z_im;
      Z_im = 2*Z_re*Z_im + c_im;
      Z_re = Z_reS - Z_imS + c_re;    
      if(Z_reS + Z_imS > 4)
	{ 
	  colorind = n;
	  break;
	}
    }
  return colorind;
}


int start_image( int maxval, int xsize, int ysize, const char * image_name )
{

  FILE* image_file;

  image_file = fopen(image_name, "w");

  int color_depth = 1 + ( (maxval>>8)>0 );

  int len = fprintf( image_file, "P5\n%d %d\n%d\n", xsize, ysize, maxval );

  fclose( image_file );

  return len;

}
