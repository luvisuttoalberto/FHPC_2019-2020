#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#define USE MPI

int main(int argc, char ** argv){

	int tag = 123;
	int rank, numproc, proc;
	long long int sum;
	long long int local_sum = 0;
	int master = 0;
	double r_start_time, r_end_time, r_total_time;
	double start_time, end_time, total_time;
	//double init_start_time, init_end_time;
	//double fin_start_time, fin_end_time;
	double comm_start_time, comm_end_time, comm_total_time;
	double comp_start_time, comp_end_time, comp_total_time;
	MPI_Status status;
	//init_start_time= MPI_Wtime();

	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	//init_end_time = MPI_Wtime();
	if(argc<=1){
		fprintf(stderr, "Usage: mpi -np n %s number_of_operations", argv[0]);
		MPI_Finalize();
		exit(-1);
	}
	r_start_time = MPI_Wtime();
	long long int N = atoll(argv[1]);
	r_end_time = MPI_Wtime();
	long long int dist, start, stop, r;
	start_time = MPI_Wtime();
	r = N%numproc;
	if(r == 0){
		dist = N/numproc;
		start = rank*dist;
		stop = start + dist;
	}else{
		if(rank < r){
			dist = N/numproc +1;
			start = rank*dist;
			stop = start + dist;
		}
		else{
			dist = N/numproc;
			start = r*(dist+1) + (rank-r)*dist;
			stop = start + dist;
		}
	}
	//This is done by all the processes, master included
	comp_start_time = MPI_Wtime();
	for(int i = start; i<stop; i++){
		local_sum += i;
	}
	comp_end_time = MPI_Wtime();
	printf("[process %d] local_sum = %lld \n", rank, local_sum);
	
	comm_start_time = MPI_Wtime();
	MPI_Reduce(&local_sum, &sum, 1, MPI_LONG_LONG, MPI_SUM, master, MPI_COMM_WORLD);
	comm_end_time = MPI_Wtime();
	if(rank == master){
		printf("The final sum is %lld \n", sum);
	}
	
	end_time = MPI_Wtime();
	total_time = end_time - start_time;
	//init_total_time = init_end_time - init_start_time;
	
	r_total_time= r_end_time - r_start_time;
	comm_total_time= comm_end_time - comm_start_time;
	comp_total_time= comp_end_time - comp_start_time;
	
	printf("[process %d] reading time: %10.10f \n", rank, r_total_time);
	printf("[process %d] communication time: %10.10f \n", rank, comm_total_time);
	printf("[process %d] computation time: %10.10f \n", rank, comp_total_time);
	printf("[process %d] total time: %10.10f \n", rank, total_time);
	//printf("[process %d] init total time: %10.10f \n", rank, init_total_time);
	
	//fin_start_time = MPI_Wtime();
	MPI_Finalize() ;
	//fin_end_time = MPI_Wtime();

	//fin_total_time = fin_end_time - fin_start_time;
	//printf("[process %d] fin total time: %10.10f \n", rank, fin_total_time);
	return 0;
}