/**
 * This code is based on http://chpc.wustl.edu/mpi-c.html
 * 
 * Copyright (C) David Castells-Rufas <david.castells@uab.es>, CEPHIS, Universitat Autonoma de Barcelona  
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>
#include <stdlib.h>
#include "../mpi.h"

/**
 * 
 * @param argc
 * @param argv
 * @return 
 */
int mpi_pi_fp_main(int argc, char *argv[]) 
{
  int myid,nprocs;
  int err;
  
  long long int npts = 1280000000;
  long long int i,mynpts;

  double f,sum,mysum;
  double xmin,xmax,x;

  double t0, tf;
  
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD,&myid);
  
  if (myid == 0) 
  {
	  MPI_enterCriticalSection();
	  printf("INFO: Computing PI (Floating Point Version) with %d processors and %d points\n", nprocs, npts);
	  MPI_leaveCriticalSection();
      t0 = MPI_Wtime();
        mynpts = npts - (nprocs-1)*(npts/nprocs);
  } 
  else 
  {
        mynpts = npts/nprocs;
  }

  mysum = 0.0;
  xmin = 0.0;
  xmax = 1.0;

  MPI_enterCriticalSection();
  printf("cpu[%d] srand\n", mpi_rank());
  MPI_leaveCriticalSection();

  srand(myid);

  for (i=0; i<mynpts; i++) 
  {
    x = (double) rand()/RAND_MAX*(xmax-xmin) + xmin;
    mysum += 4.0/(1.0 + x*x);
  }
  
  /*MPI_enterCriticalSection();
  printf("cpu[%d] mysum = %g\n", mpi_rank(), mysum);
  MPI_leaveCriticalSection();*/
  
  err = MPI_Reduce(&mysum, &sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  
  if (err != MPI_SUCCESS)
      printf("cpu[%d] ERROR %d\n", mpi_rank(), err);
  
  if (myid == 0)
  {
    f = sum/npts;
    
    tf = MPI_Wtime();

    printf("PI calculated with %lld points = %g \n",npts,f);
    printf("PI Computed in %g seconds\n", (tf-t0));
    
  }
  

  MPI_Finalize();


}


/**
 * Integer computation of PI based on montecarlo sampling
 * 
 * @param argc
 * @param argv
 * @return 
 */
int mpi_pi_montecarlo_int_main(int argc, char *argv[]) 
{
  int myid,nprocs;
  int err;
  
  long long int npts = 128000000L;
  long long int i,mynpts;

  
  unsigned int x, y, z;
  
  double f;
  double t0, tf;
  long long count = 0;
  long long finalCount = 0;
  
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD,&myid);
  
  if (myid == 0) 
  {
	  MPI_enterCriticalSection();
	  printf("INFO: Computing PI (Integer Version) with %d processors and %lld points\n", nprocs, npts);
	  MPI_leaveCriticalSection();
      t0 = MPI_Wtime();
        mynpts = npts - (nprocs-1)*(npts/nprocs);
  } 
  else 
  {
        mynpts = npts/nprocs;
  }

  
  MPI_enterCriticalSection();
  printf("cpu[%d] srand\n", mpi_rank());
  MPI_leaveCriticalSection();

  srand(myid);

#define FIXED_POINT_BITS    15
#define FIXED_POINT_ONE     (1<<FIXED_POINT_BITS)
  
  for ( i=0; i<mynpts; i++) 
  {
      x = rand() % FIXED_POINT_ONE;
      y = rand() % FIXED_POINT_ONE;
      z = ((x*x)>>FIXED_POINT_BITS)+((y*y)>>FIXED_POINT_BITS);
      if (z<=FIXED_POINT_ONE) count++;
      
      /*printf("cpu[%d] x = %g   y = %g  z= %g count = %lld\n", mpi_rank(), 
              (double) (x/(double)FIXED_POINT_ONE),
              (double) (y/(double)FIXED_POINT_ONE),
              (double) (z/(double)FIXED_POINT_ONE),
              count);*/
  }
  
  err = MPI_Reduce(&count, &finalCount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  
  if (err != MPI_SUCCESS)
      printf("cpu[%d] ERROR %d\n", mpi_rank(), err);
  
  
   
   
  
  
  /*MPI_enterCriticalSection();
  printf("cpu[%d] mysum = %g\n", mpi_rank(), mysum);
  MPI_leaveCriticalSection();*/
  
  
  if (myid == 0)
  {
      //printf("final count %lld\n", finalCount );
    f=(double)finalCount/npts*4.0;
    
    tf = MPI_Wtime();

    printf("PI calculated with %lld points = %g \n", npts,f);
    printf("PI Computed in %g seconds\n", (tf-t0));
    
  }
  

  MPI_Finalize();


}


/**
 * Full swipe of the fixed point integer range
 * 
 * @param argc
 * @param argv
 * @return 
 */
int mpi_pi_full_int_main(int argc, char *argv[]) 
{
  int rank,size;
  int err;
  
  long long int i;

  
  unsigned int x, y, z, xstart, xend;
  
  double f;
  double t0, tf;
  long long count = 0;
  long long finalCount = 0;
  
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&size);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  
  if (rank == 0) 
  {
	  MPI_enterCriticalSection();
	  printf("INFO: Computing PI (Integer Version) with %d processors\n", size);
	  MPI_leaveCriticalSection();
      t0 = MPI_Wtime();
  } 

  


#define FIXED_POINT_BITS    15
#define FIXED_POINT_ONE     (1<<FIXED_POINT_BITS)

  xstart = (FIXED_POINT_ONE / size) * rank;
  xend = xstart + (FIXED_POINT_ONE/size);
  
  if (xend > FIXED_POINT_ONE)
      xend = FIXED_POINT_ONE;
    
  for (x = xstart; x < xend; x++)
      for (y = 0; y < FIXED_POINT_ONE; y++)
  {
      z = ((x*x)>>FIXED_POINT_BITS)+((y*y)>>FIXED_POINT_BITS);
      if (z<=FIXED_POINT_ONE) count++;
      
      /*printf("cpu[%d] x = %g   y = %g  z= %g count = %lld\n", mpi_rank(), 
              (double) (x/(double)FIXED_POINT_ONE),
              (double) (y/(double)FIXED_POINT_ONE),
              (double) (z/(double)FIXED_POINT_ONE),
              count);*/
  }
  
  err = MPI_Reduce(&count, &finalCount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
  
  if (err != MPI_SUCCESS)
      printf("cpu[%d] ERROR %d\n", mpi_rank(), err);
  
  
   
   
  
  
  /*MPI_enterCriticalSection();
  printf("cpu[%d] mysum = %g\n", mpi_rank(), mysum);
  MPI_leaveCriticalSection();*/
  
  
  if (rank == 0)
  {
      //printf("final count %lld\n", finalCount );
    f=(double)finalCount/(FIXED_POINT_ONE*FIXED_POINT_ONE)*4.0;
    
    tf = MPI_Wtime();

    printf("PI calculated with %d points = %g \n", (FIXED_POINT_ONE*FIXED_POINT_ONE),f);
    printf("PI Computed in %g seconds\n", (tf-t0));
    
  }
  

  MPI_Finalize();


}