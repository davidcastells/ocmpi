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

int mpi_pi_main(int argc, char *argv[]) 
{
  int myid,nprocs;
  int err;
  
  long long int npts = 1280;
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
	  printf("INFO: Computing PI with %d processors and %d points\n", nprocs, npts);
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
