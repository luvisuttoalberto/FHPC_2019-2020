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
	double comm2_start_time, comm2_end_time, comm2_total_time;
	double comm1_start_time, comm1_end_time, comm1_total_time;
	double comp_start_time, comp_end_time, comp_total_time;
	
	MPI_Status status;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if(argc<=1){
		fprintf(stderr, "Usage: mpi -np n %s number_of_operations", argv[0]);
		MPI_Finalize();
		exit(-1);
	}
	long long int N;
	
	if(rank == 0){
		r_start_time = MPI_Wtime();
		N = atoll(argv[1]);
		r_end_time = MPI_Wtime();
		comm1_start_time = MPI_Wtime();
		for(proc = 1; proc<numproc; proc++){
			MPI_Send(&N , 1 ,MPI_LONG_LONG, proc , tag ,MPI_COMM_WORLD) ;
		}
		comm1_end_time = MPI_Wtime();
	}else{
		MPI_Recv(&N,1,MPI_LONG_LONG,master,tag,MPI_COMM_WORLD,&status);
	}



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
	comp_start_time = MPI_Wtime();
	for(int i = start; i<stop; i++){
		local_sum += i;
	}
	comp_end_time = MPI_Wtime();
	printf("[process %d] local_sum = %lld \n", rank, local_sum);
	
	if(rank!=master){
		comm2_start_time = MPI_Wtime();
		MPI_Send(&local_sum, 1, MPI_LONG_LONG, master, tag, MPI_COMM_WORLD);
		comm2_end_time = MPI_Wtime();
	}
	else{
		sum = local_sum;
		comm2_start_time = MPI_Wtime();
		for(proc = 1; proc<numproc; proc++){
			MPI_Recv(&local_sum, 1, MPI_LONG_LONG, proc, tag, MPI_COMM_WORLD, &status);
			sum+=local_sum;	
		}
		comm2_end_time = MPI_Wtime();
		printf("The final sum is %lld \n", sum);
	}
	end_time = MPI_Wtime();
	total_time = end_time - start_time;
	
	r_total_time= r_end_time - r_start_time;
	comm2_total_time= comm2_end_time - comm2_start_time;
	comm1_total_time= comm1_end_time - comm1_start_time;
	comp_total_time= comp_end_time - comp_start_time;
	if(rank == 0){
		printf("[process %d] reading time: %10.10f \n", rank, r_total_time);
	}
	printf("[process %d] first communication time: %10.10f \n", rank, comm1_total_time);
	printf("[process %d] second communication time: %10.10f \n", rank, comm2_total_time);
	printf("[process %d] computation time: %10.10f \n", rank, comp_total_time);
	printf("[process %d] total time: %10.10f \n", rank, total_time);
	
	MPI_Finalize() ;
	return 0;
}