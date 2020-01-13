#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#define USE MPI
//comment this define if you want to read N from the command line
#define READFROMFILE

int main(int argc, char ** argv){

	int tag = 123;
	int rank, numproc;
	long long int N;
	unsigned long long int sum, dist, start, stop, r, proc, i;
	unsigned long long int local_sum = 0;
	int master = 0;
	double r_start_time, r_end_time, r_total_time;//reading time
	double start_time, end_time, total_time;//total time
	double comm_start_time, comm_end_time, comm_total_time;// communication time
	double comp_start_time, comp_end_time, comp_total_time; // computation time
	
	MPI_Status status;
	MPI_Init(&argc,&argv);
	MPI_Comm_size(MPI_COMM_WORLD, &numproc);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	#ifdef READFROMFILE

	FILE *fd;
	start_time = MPI_Wtime(); // begin to measure the total time

	r_start_time = MPI_Wtime(); //begin to measure the reading time
	fd = fopen("N.txt", "r");
	fscanf(fd, "%llu", &N);
	r_end_time = MPI_Wtime(); // finish to measure the reading time
	fclose(fd);

#else
	//Verify that we receive the number of operations to do from the input
	if(argc<=1){
		fprintf(stderr, "Usage: mpi -np n %s number_of_operations", argv[0]);
		MPI_Finalize();
		exit(-1);
	}
	start_time = MPI_Wtime(); // begin to measure the total time
	//Read N as an input; done by all processes
	r_start_time = MPI_Wtime(); //begin to measure the reading time
	N = atoll(argv[1]);
	r_end_time = MPI_Wtime(); // finish to measure the reading time

#endif
	
	r = N%numproc;

	//Here we have the division of the numbers to sum between all the processes
	//the remainder is distributed as one operation for every process, until the remainder is finished
	//in this way the difference between the "unlucky" processes and the "lucky" ones is only one sum
	//in this way we try to avoid load imbalance
	if(r == 0){
		dist = N/numproc;
		start = rank*dist+1;
		stop = start + dist;
	}else{
		if(rank < r){// if the process is unlucky it does one more sum than the others
			dist = N/numproc +1;
			start = rank*dist+1;
			stop = start + dist;
		}
		else{
			dist = N/numproc;
			start = r*(dist+1) + (rank-r)*dist+1;// if the process is lucky, we have to consider that r previous processes has done one more sum than he'll do
			stop = start + dist;
		}
	}

	//This is done by all the processes, master included
	comp_start_time = MPI_Wtime();// begin to measure computation time
	for(i = start; i<stop; i++){
		local_sum += i;
	}
	comp_end_time = MPI_Wtime(); // finish to measure computation time

	printf("[process %d] local_sum = %llu \n", rank, local_sum);
	
	//performing the communication of the local_sums and the final sum
	//using the reduce collective mpi operation 
	comm_start_time = MPI_Wtime();// begin to measure communication time
	MPI_Reduce(&local_sum, &sum, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, master, MPI_COMM_WORLD);
	comm_end_time = MPI_Wtime();// finish to measure communication time
	
	end_time = MPI_Wtime();//finish to measure the total time
	
	if(rank == master){
		printf("Numproc = %d \n Sum = %llu \n", numproc, sum);
	}
	

	// calculation of the total times
	total_time = end_time - start_time;
	r_total_time= r_end_time - r_start_time;
	comm_total_time= comm_end_time - comm_start_time;
	comp_total_time= comp_end_time - comp_start_time;
	
	//printing the results about time
	printf("[process %d] reading time: %10.10f \n", rank, r_total_time);
	printf("[process %d] communication time: %10.10f \n", rank, comm_total_time);
	printf("[process %d] computation time: %10.10f \n", rank, comp_total_time);
	printf("[process %d] total time: %10.10f, \n", rank, total_time);
		
	MPI_Finalize() ;
	
	return 0;
}