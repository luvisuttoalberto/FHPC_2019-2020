#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#define USE MPI

int main(int argc, char ** argv){

	int tag = 123;
	int rank, numproc, proc;
	long long int sum, N, dist, start, stop, r;
	long long int local_sum = 0;
	int master = 0;
	double r_start_time, r_end_time, r_total_time;//reading time
	double start_time, end_time, total_time;//total time
	double comm1_start_time, comm1_end_time, comm1_total_time;// first communication time
	double comm2_start_time, comm2_end_time, comm2_total_time;// second communication time
	double comp_start_time, comp_end_time, comp_total_time; // computation time
	
	MPI_Status status;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//Verify that we receive the number of operations to do from the input
	if(argc<=1){
		fprintf(stderr, "Usage: mpi -np n %s number_of_operations", argv[0]);
		MPI_Finalize();
		exit(-1);
	}
	
	//read N as an input; done by only the master
	if(rank == 0){
		r_start_time = MPI_Wtime(); //begin to measure the reading time
		N = atoll(argv[1]);
		r_end_time = MPI_Wtime(); // finish to measure the reading time
	}

	//communicate N to all the slaves using the broadcast collective mpi operation 
	comm1_start_time = MPI_Wtime(); // begin to measure the first communication time
	MPI_Bcast(&N, 1, MPI_LONG_LONG, master, MPI_COMM_WORLD);
	comm1_end_time = MPI_Wtime();  // finish to measure the first communication time

	start_time = MPI_Wtime();// begin to measure the total time
	r = N%numproc;
	//Here we have the division of the numbers to sum between all the processes
	//the remainder is distributed as one operation for every process, until the remainder is finished
	//in this way the difference between the "unlucky" processes and the "lucky" ones is only one sum
	//in this way we try to avoid load imbalance
	if(r == 0){
		dist = N/numproc;
		start = rank*dist;
		stop = start + dist;
	}else{
		if(rank < r){// if the process is unlucky it does one more sum than the others
			dist = N/numproc +1;
			start = rank*dist;
			stop = start + dist;
		}
		else{
			dist = N/numproc;
			start = r*(dist+1) + (rank-r)*dist;// if the process is lucky, we have to consider that r previous processes has done one more sum than he'll do
			stop = start + dist;
		}
	}

	//This is done by all the processes, master included
	comp_start_time = MPI_Wtime();// begin to measure computation time
	for(int i = start; i<stop; i++){
		local_sum += i;
	}
	comp_end_time = MPI_Wtime();// finish to measure computation time

	printf("[process %d] local_sum = %lld \n", rank, local_sum);
	
	//performing the communication of the local_sums and the final sum
	//using the reduce collective mpi operation 
	comm2_start_time = MPI_Wtime();// begin to measure second communication time
	MPI_Reduce(&local_sum, &sum, 1, MPI_LONG_LONG, MPI_SUM, master, MPI_COMM_WORLD);
	comm2_end_time = MPI_Wtime();// finish to measure second communication time
	
	end_time = MPI_Wtime();//finish to measure the total time

	if(rank == master){
		printf("The final sum is %lld \n", sum); // printing of the result
	}


	// calculation of the total times
	total_time = end_time - start_time;
	r_total_time= r_end_time - r_start_time;
	comm1_total_time= comm1_end_time - comm1_start_time;
	comm2_total_time= comm2_end_time - comm2_start_time;
	comp_total_time= comp_end_time - comp_start_time;

	//printing the results about time
	if(rank == 0){// only the master prints out the reading time because it's the only one reading
		printf("[process %d] reading time: %10.10f \n", rank, r_total_time);
	}
	//printf("[process %d] first communication time: %10.10f \n", rank, comm1_total_time);
	//printf("[process %d] second communication time: %10.10f \n", rank, comm2_total_time);
	printf("[process %d] total communication time: %10.10f \n", rank, comm1_total_time + comm2_total_time);
	printf("[process %d] computation time: %10.10f \n", rank, comp_total_time);
	printf("[process %d] total time: %10.10f \n", rank, total_time);
	
	MPI_Finalize() ;
	return 0;
}