/**
 * Copyright (C) David Castells-Rufas, CEPHIS, Universitat Autonoma de Barcelona  
 * david.castells@uab.cat
 * 
 * This work was used in the publication of 
 * "128-core Many-Soft-Core Processor with MPI support" 
 * available online on 
 * https://www.researchgate.net/publication/282124163_128-core_Many-Soft-Core_Processor_with_MPI_support
 * 
 * I encourage that you cite it as:
 * [*] Castells-Rufas, David, and Jordi Carrabina. "128-core Many-Soft-Core Processor with MPI support." 
 * Jornadas de Computación Reconfigurable y Aplicaciones (JCRA) (2015).
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
/*
 * mpiqueens2.c
 *
 * https://github.com/victoraldecoa/N-Queens-Solver_OpenMP_Example/blob/master/src/nqueens-openmp.c
 *
 *  Created on: 09/12/2014
 *      Author: test64
 */
#include <stdio.h>
#include <stdlib.h>



#ifdef WIN32
    #include "../arch/windows/mutex.h"
    #include "../arch/windows/cpuid.h"
#endif

#ifdef LINUX
    #include "../arch/linux/mutex.h"
    #include "../arch/linux/cpuid.h"
#endif

#include "../mpi.h"


int verbose = 0;

#define MAX_N 16

int printQueens(int* queens, int n)
{
	char str[200];
	char c = 0;
        int i;

	str[c++] = '[';

	for (i=0; i < n; i++)
	{
		str[c++] = queens[i] + '0';
		str[c++] = ',';
	}

	c--;

	str[c++] = ']';
	str[c++] = 0;

	printf("%s\n", str);
        
        return 0;
}

int check_acceptable(int queen_rows[MAX_N], int n)
{
//	printQueens(queen_rows, n);

	int i, j;
	for (i = 0; i < n; i++)
	{
//		j++;
		for (j = i+1; j < n; j++)
		{
			// two queens in the same row => not a solution!
			if (queen_rows[i] == queen_rows[j]) return 0;

			// two queens in the same diagonal => not a solution!
			if (queen_rows[i] - queen_rows[j] == i - j ||
			    queen_rows[i] - queen_rows[j] == j - i)
			    return 0;
		}
	}

	return 1;
}

int mpiqueens2_sequential(int n);
int mpiqueens2_master(int n);
int mpiqueens2_slave();

#define TAG_ASK_WORK	1
#define TAG_SEND_WORK	2

#define min(a, b)	((a>b)?(b):(a))

#define QUEENS	8


#ifdef USING_OC_MPI
int mpiqueens2_main(int argc, char* args[])
{
	int size, rank;

        printf("Hello from %d\n", mpi_rank());
        
	MPI_Init(&argc, &args);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

        printf("rank = %d size=%d \n", rank, size);
        
	if (size == 1)
		mpiqueens2_sequential(QUEENS);
	else if (rank == 0)
		mpiqueens2_master(QUEENS);
	else
		mpiqueens2_slave();

	MPI_Finalize();

	return 0;
}

#else
int main(int argc, char* args[])
{
	int size, rank;

	MPI_Init(&argc, &args);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	if (size == 1)
		mpiqueens2_sequential(QUEENS);
	else if (rank == 0)
		mpiqueens2_master(QUEENS);
	else
		mpiqueens2_slave();

	MPI_Finalize();

	return 0;
}
#endif

int mpiqueens2_slave()
{
	int number_solutions = 0;
	int workload[3];
	int iter, max_iter, init, n;
	MPI_Status status;

#ifdef USING_OC_MPI
	MPI_enterCriticalSection();
	MPI_AddDebugLine('q', 0);
	MPI_leaveCriticalSection();
#endif

	// ask for work
	while(1)
	{
/*#ifdef USING_OC_MPI
		MPI_enterCriticalSection();
		MPI_AddDebugLine('q', 1);
		MPI_leaveCriticalSection();
#endif*/

	MPI_Send(&number_solutions, 1, MPI_INT, 0, TAG_ASK_WORK, MPI_COMM_WORLD);

/*#ifdef USING_OC_MPI
		MPI_enterCriticalSection();
		MPI_AddDebugLine('q', 2);
		MPI_leaveCriticalSection();
#endif*/

	number_solutions = 0;

	MPI_Recv(&workload, 3, MPI_INT, 0, TAG_SEND_WORK, MPI_COMM_WORLD, &status);

	init = workload[0];
	max_iter = workload[1];
	n = workload[2];

	if (init == -1)
		return 0;

#ifdef WIN32
	printf("[cpu%d] slave iterating from %d to %d for n %d\n", mpi_rank(), init, max_iter, n);
#endif
      
#ifdef LINUX
	if (verbose)
            printf("[cpu%d] slave iterating from %d to %d for n %d\n", mpi_rank(), init, max_iter, n);
#endif
	for (iter = init; iter < max_iter; iter++)
		{
			int code = iter;
			int i;
		    int queen_rows[MAX_N];
			// the index correspond to the queen's number and the queen's collumn
			// we only generate configurations where there's only one queen per collumn
			for (i = 0; i < n; i++)
			{
				queen_rows[i] = code % n;

				code /= n;
			}

			if (check_acceptable(queen_rows, n))
			{
	//#pragma omp atomic
			    number_solutions++;
	//		    printf("slave solutions %d\n", number_solutions);
	//#pragma omp critical
	/*            {
				    printf("\n");
				    for (i = 0; i < n; i++)
				    {
				        int j;
					    for (j = 0; j < n; j++)
					    {
						    if (queen_rows[i] == j)	printf("|X");
						    else printf("| ");
					    }
					    printf("|\n");
				    }
				    printf("\n");
				}*/
			}
		}
	}

	return 0;
}



int mpiqueens2_master(int n)
{

    int max_iter = 1;

    double start_time, end_time;
    int number_solutions = 0;

    int size;

    MPI_Comm_size(MPI_COMM_WORLD, &size);

	{
        int i;


        for (i = 0; i < n; i++)
        {
            max_iter *= n;
        }
    }

	printf("Starting nqueens %d x %d max iter: %d with %d processors\n", n, n, max_iter,size );

    start_time = MPI_Wtime();

	int iter = 0;
	int dead = 0;
	//int iterStep = (max_iter / 100000) * size;
        int iterStep = (max_iter / (size *100) ) ;

//#pragma omp parallel for
	do
	{
		MPI_Status status;
		MPI_Request request;
		int error;
		int result;
		int dst;
		int workload[3];

/*		MPI_enterCriticalSection();
		MPI_Debug_Report();
		MPI_DumpDebug();
		MPI_leaveCriticalSection();*/



/*#ifdef USING_OC_MPI
		error = MPI_Irecv(&result, 1, MPI_INT, MPI_ANY_SOURCE, TAG_ASK_WORK, MPI_COMM_WORLD, &request);
		int flag;

		do
		{
			MPI_Test(&request, &flag, &status);

			if (!flag)
			{
/ *				MPI_enterCriticalSection();
				MPI_Debug_Show_Messages();
				MPI_DumpDebug();
				MPI_leaveCriticalSection();

				mpi_sleepMilliseconds(1000);* /
			}
		} while (!flag);
#else*/
		error = MPI_Recv(&result, 1, MPI_INT, MPI_ANY_SOURCE, TAG_ASK_WORK, MPI_COMM_WORLD, &status);
//#endif


		number_solutions += result;
		dst = status.MPI_SOURCE;

                if (verbose)
                    printf("slave [%d] asking for work. Solutions= %d\n", dst, result);

		if (iter >= max_iter)
		{
			dead++;
			workload[0] = -1;
			MPI_Send(workload, 3, MPI_INT, dst, TAG_SEND_WORK, MPI_COMM_WORLD);
		}
		else
		{
			workload[0] = iter;
			workload[1] = min(iter+iterStep, max_iter);
			workload[2] = n;

			MPI_Send(workload, 3, MPI_INT, dst, TAG_SEND_WORK, MPI_COMM_WORLD);

			iter = workload[1];
		}
	} while (dead < size-1);



    // get end time
    end_time = MPI_Wtime();
    // print results
    printf("The execution time is %g sec\n", end_time - start_time);
    printf("Number of found solutions is %d\n", number_solutions);

	return 0;
}

int mpiqueens2_sequential(int n)
{
    int max_iter = 1;

    double start_time, end_time;
    int number_solutions = 0;

	{
        int i;


        for (i = 0; i < n; i++)
        {
            max_iter *= n;
        }
    }

	printf("Starting nqueens %d x %d max iter: %d\n", n, n, max_iter);

    start_time = MPI_Wtime();

	int iter;
//#pragma omp parallel for
	for (iter = 0; iter < max_iter; iter++)
	{
		int code = iter;
		int i;
	    int queen_rows[MAX_N];
		// the index correspond to the queen's number and the queen's collumn
		// we only generate configurations where there's only one queen per collumn
		for (i = 0; i < n; i++)
		{
			queen_rows[i] = code % n;

			code /= n;
		}

		if (check_acceptable(queen_rows, n))
		{
//#pragma omp atomic
		    number_solutions++;
		    printf("%d\n", number_solutions);
//#pragma omp critical
/*            {
			    printf("\n");
			    for (i = 0; i < n; i++)
			    {
			        int j;
				    for (j = 0; j < n; j++)
				    {
					    if (queen_rows[i] == j)	printf("|X");
					    else printf("| ");
				    }
				    printf("|\n");
			    }
			    printf("\n");
			}*/
		}
	}

    // get end time
    end_time = MPI_Wtime();
    // print results
    printf("The execution time is %g sec\n", end_time - start_time);
    printf("Number of found solutions is %d\n", number_solutions);

	return 0;
}



int main(int argc, char** argv) 
{
    initMutex();
    
    createSlaveThreads(mpiqueens2_main);
    return mpiqueens2_main(argc, argv);
}