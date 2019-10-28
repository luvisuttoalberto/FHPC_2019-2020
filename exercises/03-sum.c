#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#define USE MPI

int main(int argc, char ** argv){

	int tag = 123;
	int rank, numproc, proc;
	float sum;
	float local_sum = 0;
	int master = 0;
	MPI_Status status;

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(argc<=1){
		fprintf(stderr, "Usage: mpi -np n %s number_of_operations", argv[0]);
		MPI_Finalize();
		exit(-1);
	}
	long long int N = atoll(argv[1]);
	long long int dist = N/numproc;
	float numeri[N];
	if(rank == 0){
		for(int i = 0; i<N; i++){
			numeri[i] = i;
		}
		for(proc = 1; proc<numproc; proc++){
			MPI_Send(&numeri, N, MPI_LONG_LONG, proc, tag, MPI_COMM_WORLD);
		}		
	}
	int start = rank*dist;
	int stop = start + dist;
	if(rank!=master){
		MPI_Recv(&numeri, N, MPI_LONG_LONG, master, tag, MPI_COMM_WORLD, &status);
	}
	//This is done by all the processes, master included
	for(int i = start; i<stop; i++){
		local_sum += numeri[i];
	}

	if(rank!=master){
		MPI_Send(&local_sum, 1, MPI_FLOAT, master, tag, MPI_COMM_WORLD);
	}
	else{
		sum = local_sum;
		for(proc = 1; proc<numproc; proc++){
			MPI_Recv(&local_sum, 1, MPI_FLOAT, proc, tag, MPI_COMM_WORLD, &status);
			sum+=local_sum;	
		}
		printf("The final sum is %f", sum);
	}

	MPI_Finalize() ;
	return 0;
}