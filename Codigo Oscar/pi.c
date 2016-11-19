/*
  Large Scale Computing
  OpenMP Sample ex35.c
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mpi.h"
#include "omp.h"
 
int main (int argc, char *argv[]) {

 double start_time, run_time;
    

  int myid,numproc;
  int i,n;
  int mystart,myend;
  double x,pi4;
 
  /* Initialize */
  MPI_Init(&argc,&argv);
 
  /* get myid and # of processors */
  MPI_Comm_size(MPI_COMM_WORLD,&numproc);
  MPI_Comm_rank(MPI_COMM_WORLD,&myid);
 
  if (argc < 2){
    if (myid == 0)  printf("%s [number of terms]\n",argv[0]);
    MPI_Finalize();
    exit(-1);
  }
 
  /* get # of terms */
  n = atoi(argv[1]);
  if (myid == 0) printf("Start n=%d\n",n);
 
  /* divide loop */
  mystart = (n / numproc) * myid;
  if (n % numproc > myid){
    mystart += myid;
    myend = mystart + (n / numproc) + 1;
  }else{
    mystart += n % numproc;
    myend = mystart + (n / numproc);
  }
  printf("CPU%d %d ~ %d\n",myid,mystart,myend);
 
  /* compute PI */
  x=0.0;
  for (i = mystart ; i < myend ; i++){
    x += pow(-1.0,(double)i) / (double)(2 * i + 1);
  }
 
  /* sum up */
  pi4 = 0.0;
  start_time = omp_get_wtime();
  MPI_Reduce(&x, &pi4, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  run_time = omp_get_wtime() - start_time;	
  if (myid == 0) printf("Pi=%1.16f\n",4.0 * pi4);
  printf("FinishTime %lf\n", run_time);
 
  MPI_Finalize();
}
