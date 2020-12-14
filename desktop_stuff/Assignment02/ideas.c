//suppose we know that N is a multiple of the number of threads
// Is this necessary? We don't suppose it for the initialization but it works perfectly the same
//Because the schedule deals automatically with the remainder
//Does it also works with the allocation? Don't think so
int n = N/nthreads;
#pragma omp parallel for
{
	double *array = (double*)malloc( n * sizeof(double) );
  	for ( int ii = 0; ii < N; ii++ ){
    	array[ii] = (double)ii;
  	}
}
//do we need to allocate the memory on the heap? 
//Could we instead make every thread initialize his own piece of the array and perform the sum, and then aggregate?
//But how would we exploit Openmp then?

//the following n also could be initialized inside of a parallel region, so that we have it private for every thread (and we can deal with the remainder)
int n = N/nthreads;
#pragma omp parallel
  {
  	double * array = (double*)malloc( n * sizeof(double) );


  }