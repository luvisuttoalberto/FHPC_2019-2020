// define some dummy variables to make code more readable                                                                                              
  int im_ready_tag = 0;
  int workpile_tag = 1;
  int special_tag = 42;
  int finish_message = -1;
  int root = 0;

  if(myid == 0){
    int line = 1;
    int worker;
    while(line <= n_y ){
    // listen who says 'Im ready'                                                                                                                        
    MPI_Recv(&worker, 1, MPI_INT, MPI_ANY_SOURCE, im_ready_tag, MPI_COMM_WORLD, &status);
    //                     printf("Master heard %d is ready to work\n", worker);                                                                         
    // send him line line to work on                                                                                                                     
    MPI_Send(&line, 1, MPI_INT, worker, workpile_tag, MPI_COMM_WORLD);
    ++line;
    }
    // send message with special tag to say we're done                                                                                                   
    for(int slave=1; slave <= numproc; ++slave){
      MPI_Send(&finish_message, 1, MPI_INT, slave, special_tag, MPI_COMM_WORLD);
    } // try to see if {} are needed                                                                                                                     
  }
  else{
    int line_num;
    unsigned char* line = (unsigned char*)malloc( n_y*sizeof(unsigned char) );
    while( 1 ){
      // say to master you're ready to work                                                                                                              
      MPI_Send(&myid, 1, MPI_INT, root, im_ready_tag, MPI_COMM_WORLD);
      // listen your next job                                                                                                                            
      MPI_Recv(&line_num, 1, MPI_INT, root, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      //                           printf("%d understood line to work is \t\t\t%d\n", myid, line_num);                                                   
      if(line_num == finish_message){
        //                        printf("%d will BREAK NOW\n", myid);                                                                                   
        break;
      }
      // work with line_num                                                                                                                              
      heavy_work();
      // write your results                                                                                                                              
    }
  }