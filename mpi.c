/*
 * mpi.c
 *
 *  Created on: 12/11/2013
 *      Author: test64
 */

#include "mpi.h"

#include "config.h"

#ifdef WIN32
#include "arch/windows/PerformanceCounter.h"
#include "arch/windows/mutex.h"
#include "arch/windows/cpuid.h"
#include "arch/windows/CacheBypass.h"
#endif

#ifdef NIOS
#include <nios2.h>
#include "arch/nios/PerformanceCounter.h"
#include "arch/nios/mutex.h"
#include "arch/nios/cpuid.h"
#include "arch/nios/CacheBypass.h"
#endif

#ifdef LINUX
#include "arch/linux/PerformanceCounter.h"
#include "arch/linux/mutex.h"
#include "arch/linux/cpuid.h"
#include "arch/linux/CacheBypass.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// initialization
//int	MPI_Process_level[NUM_OF_PROCESSORS];			// current level of each process
//int MPI_Process_waiting[NUM_OF_PROCESSORS - 1];		// the waiting process of each level 1..N



char MPI_Last_Error[200];
//int MPI_master_init = 0;
int MPI_Barrier_Count = 0;
int MPI_Barrier_Exit_Count = NUM_OF_PROCESSORS;


int MPI_initialized[NUM_OF_PROCESSORS];		// an array with the initialization status of each processor


// chunks are next, prev, size (negative means free) , data


double MPI_Rx_Timeout = 0;		// rx timeout in seconds
double MPI_Tx_Timeout = 10;		// rx timeout in seconds

int mpi_global_error = 0;		// global error

#define MPI_ENTER_TRACE(x)		CacheBypassWriteInt(&MPI_initialized[0], (int) x);

#define VALID_ASCII(x) 	(((x>=' ') && (x<=0x80)) ? x : '-')

void MPI_Debug_Report()
{
	int rank = mpi_rank();
	int i;

	if (rank != 0)
		return;

	for (i = 0; i < NUM_OF_PROCESSORS; i++)
	{
		int v = CacheBypassReadInt(&MPI_initialized[i]);

		printf("Processor %d initialized = %d\n", i, v);
	}
}


int MPI_Init(int* argc, char*** args)
{
	int rank = mpi_rank();
	int numInitialized = 0;


	if (rank == 0)
	{
		int i;
		//CacheBypassWriteInt(&MPI_Buffer_Count, 0);
		//CacheBypassWriteInt(&MPI_Message_Count, 0);
		mpi_clear_vars();

		//MPI_Debug_Report();

		//CacheBypassWriteInt(&MPI_master_init, 1);

		MPI_enterCriticalSection();
		printf("[CPU%d] >>MPI_Init\n", rank);
		MPI_leaveCriticalSection();

/*		for (i=0; i < NUM_OF_PROCESSORS; i++)
			printf("MPI_initialized[%d] pointer %p\n", i, &MPI_initialized[i]);
*/
	}
	//else
loop:
	MPI_enterCriticalSection();

	{
		int i;
		//CacheBypassReadInt(&MPI_Buffer_Count);
		//CacheBypassReadInt(&MPI_Message_Count);
		numInitialized = 0;

		for (i=0; i< NUM_OF_PROCESSORS; i++)
		{
			int v = CacheBypassReadInt(&MPI_initialized[i]);

                        
			if (v)
				numInitialized++;
                        

//                        printf("cache bypass initialized [%d] = %d num = %d\n", i, v, numInitialized);

		}
	}

	CacheBypassWriteInt(&MPI_initialized[rank], 1);

	printf("[cpu%d] initialized. Init count = %d\n", rank, numInitialized);


	MPI_leaveCriticalSection();

	if (numInitialized < NUM_OF_PROCESSORS)
	{
//            printf("[cpu%d] num initialized = %d\n", rank, numInitialized);
		
            if (rank == 0)
            {
//                MPI_Debug_Report();

//		mpi_sleepMilliseconds(1000);
            }
            //mpi_sleepMilliseconds(1000);
		goto loop;
	}


        if (rank == 0)
        {
        	MPI_enterCriticalSection();
		printf("[CPU%d] <<MPI_Init\n", rank);
		MPI_leaveCriticalSection();
        }


        if (rank >= SIZE_OF_MPI_APPLICATION)
        	while (1);

	return MPI_SUCCESS;
}

int MPI_Finalize()
{
	/*MPI_enterCriticalSection();
	MPI_AddDebugLine('F', 0);
	MPI_leaveCriticalSection();*/

	MPI_Barrier(MPI_COMM_WORLD);

	CacheBypassWriteInt(&MPI_initialized[mpi_rank()], 0);

	return MPI_SUCCESS;
}

int mpi_rank()
{
    RETURN_CPU_IDENTIFIER();
}

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
	*rank = mpi_rank();

	return MPI_SUCCESS;
}


int MPI_Comm_size(MPI_Comm comm,  int *size)
{
	*size = SIZE_OF_MPI_APPLICATION;
	return MPI_SUCCESS;
}


double MPI_Wtime()
{
	uint64 t0 = 0;
	uint64 tf;
	double ret;

	perfCounter(&tf);

//	printf("t0 = %16llX\n", t0);
//	printf("tf = %16llX\n", tf);

	ret = secondsBetweenLaps(t0, tf);

//	printf("ti = %f\n", ret);

	return ret;
}




/**
 * Begins a non-blocking receive, just save info, real meat is done in MPI_Wait
 */
int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request* request)
{
	request->data = buf;
	request->count = count;
	request->type = datatype;
	request->src = source;
	request->tag = tag;
	request->comm = comm;

	return MPI_SUCCESS;
}


int MPI_Test(MPI_Request* request, int* flag, MPI_Status* status)
{
	if (request == NULL)
		return MPI_ERR_REQUEST;

	int result = mpi_Irecv(request->data, request->count, request->type, request->src, request->tag, request->comm, status);

	if (result == MPI_ERR_PENDING)
		*flag = 0;
	else
		*flag = 1;

	return MPI_SUCCESS;
}

/**
 *
 */
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
	int result;
	int retries = 0;
	uint64 t0, tf;

//	MPI_ENTER_TRACE(MPI_Recv);
#ifdef WIN32
        //printf("message recv %d->%d\n", source, mpi_rank());
#endif

	if (mpi_rank() == 0 && CacheBypassReadInt(&mpi_global_error))
	{
		printf("ERROR: Global Error occurred\n");
		MPI_Debug_Show_Messages();
		while (1);
	}


	perfCounter(&t0);
loop:
	result = mpi_Irecv(buf, count, datatype, source, tag, comm, status);

	if (result == MPI_ERR_PENDING)
	{
		perfCounter(&tf);
/*		if (mpi_rank() == 0)
		{
			MPI_Debug_Report();
			mpi_sleepMilliseconds(3000);
		}*/

                

		if (MPI_Rx_Timeout != 0)
		{
			if (secondsBetweenLaps(t0, tf) > MPI_Rx_Timeout)
				return MPI_ERR_TIMEOUT;
		}

		/*
		if (retries < 100)
			retries++;
		else
			MPI_Debug_Show_Messages();

		mpi_sleepMilliseconds(retries);*/

		goto loop;
	}

	return result;
}


int MPI_Wait(MPI_Request *request, MPI_Status *status)
{
	if (request->type == MPI_REQUEST_RECV)
		return MPI_Recv(request->data, request->count, request->type, request->src, request->tag, request->comm, status);
	else
	{
		printf("ERROR: no valid request type");
		return MPI_ERR_REQUEST;
	}
}

/**
 * following http://en.wikipedia.org/wiki/Peterson's_algorithm#Filter_algorithm:_Peterson.27s_algorithm_for_N_processes
 */
void MPI_enterCriticalSection()
{
	mutexAcquire();
}

void MPI_leaveCriticalSection()
{
	mutexRelease();
}


int MPI_Type_size(MPI_Datatype datatype, int *size)
{
	switch (datatype)
	{
	case MPI_INT: *size = sizeof(int); return MPI_SUCCESS;
	case MPI_DOUBLE: *size = sizeof(double); return MPI_SUCCESS;
	default:
		return MPI_ERR_TYPE;
	}


}



    
/**
 */
int MPI_Barrier( MPI_Comm comm )
{
	int i;
	int error;
	if (mpi_rank() == 0)
	{
		for (i=1; i< NUM_OF_PROCESSORS; i++)
		{
			int token;
			MPI_Status status;
			error = MPI_Recv(&token, 1, MPI_INT, i, MPI_INTERNAL_BARRIER_TAG, comm, &status);

			if (error != MPI_SUCCESS)
			{
				printf("[ERROR] MPI_Barrier error: %d receiving from %d\n", error, i);
				//MPI_Debug_Report();
				//MPI_DumpDebug();
				MPI_Debug_Show_Messages();

			}
//			printf("barrier rcv from %d\n", i);

			error = MPI_Send(&token, 1, MPI_INT, i, MPI_INTERNAL_BARRIER_TAG, comm);

//			printf("barrier snd to %d\n", i);
		}
	}
	else
	{
		int token;
		MPI_Status status;
		MPI_Send(&token, 1, MPI_INT, 0, MPI_INTERNAL_BARRIER_TAG, comm);
		error = MPI_Recv(&token, 1, MPI_INT, 0, MPI_INTERNAL_BARRIER_TAG, comm, &status);

		if (error != MPI_SUCCESS)
			while(1);				// force to block
	}

	return MPI_SUCCESS;
}


void mpi_sleepMilliseconds(int milliseconds)
{
    uint64 t0;
    uint64 tf;
    double deadline = milliseconds / 1000.0;
    
    perfCounter(&t0);
    do
    {
        perfCounter(&tf);
    } while(secondsBetweenLaps(t0, tf) < deadline); 
}

