#include "../mpi.h"
#include "../config.h"

#include <stdio.h>

#ifdef WIN32
#include "../arch/windows/PerformanceCounter.h"
#include "../arch/windows/mutex.h"
#include "../arch/windows/cpuid.h"
#include "../arch/windows/CacheBypass.h"
#endif

#ifdef NIOS
#include <nios2.h>
#include "../arch/nios/PerformanceCounter.h"
#include "../arch/nios/mutex.h"
#include "../arch/nios/cpuid.h"
#include "../arch/nios/CacheBypass.h"
#endif

#ifdef LINUX
#include "../arch/linux/PerformanceCounter.h"
#include "../arch/linux/mutex.h"
#include "../arch/linux/cpuid.h"
#include "../arch/linux/CacheBypass.h"
#endif


#define NUM_OF_MPI_MESSAGES (NUM_OF_PROCESSORS * 10)
#define MEAN_MPI_MSG_SIZE	(0x400 * 1)

#define MPI_BUFFER_SIZE (NUM_OF_MPI_MESSAGES * MEAN_MPI_MSG_SIZE /  4)
int MPI_Buffer[MPI_BUFFER_SIZE];								// buffer where the MPI messages are copied

MPI_Msg MPI_Messages[NUM_OF_MPI_MESSAGES];
int MPI_Message_Count = 0;


#define NEXT_INDEX 	0
#define SIZE_INDEX 	1
#define DATA_INDEX 	2


char* mpi_malloc_locked(int size);

int* mpi_malloc_get_chunk(int size)
{
	int* pChunk = MPI_Buffer;

	while (1)
	{
		int chunkSize = CacheBypassReadInt(&pChunk[SIZE_INDEX]);

		if (chunkSize == 0)
			// last chunk
			return pChunk;

		if ((chunkSize < 0) && ((-chunkSize)>size))
		{
			//printf("reusing index %d needed size: %d available: %d\n", (pChunk - mpi_buffer), size, pChunk[SIZE_INDEX]);
			// pre-freed chunk with enought room
			return pChunk;
		}

		int nextIndex = CacheBypassReadInt(&pChunk[NEXT_INDEX]);

		if (nextIndex == 0)
		{
			//printf("Strange case address: %p nextIndex: %d size: %d \n", pChunk, pChunk[NEXT_INDEX], pChunk[SIZE_INDEX]);
			return NULL;
		}

		if ((nextIndex + 3) >= MPI_BUFFER_SIZE)
			return NULL;

		pChunk = &pChunk[nextIndex];

	}
}

/**
 *	allocates a memory block of the reported size, if uses free space (at the end)
 *  or reuses a freed block
 */
char* mpi_malloc_locked(int size)
{
	if (size == 0)
		return NULL;

	// Get a free chunk to use
	int* pChunk = mpi_malloc_get_chunk(size);

	if (pChunk == NULL)
        {
            //printf("MALLOC: failed to allocate chunk\n");
            return NULL;
        }


	int prevSize = CacheBypassReadInt(&pChunk[SIZE_INDEX]);
	int intSize = ((size + 3) / 4);

	int thisIndex = pChunk - MPI_Buffer;

//	printf("this index %d\n", thisIndex);

	if ((thisIndex+3+intSize) >= MPI_BUFFER_SIZE)
		return NULL;

	if (prevSize == 0)
	{
            int nextIndex = 2 + intSize;
            // if chunk is not reused assign the next pointer
            CacheBypassWriteInt(&pChunk[NEXT_INDEX], nextIndex);
            CacheBypassWriteInt(&pChunk[SIZE_INDEX], intSize);
                
            int* pNextChunk = &pChunk[nextIndex];
            CacheBypassWriteInt(&pNextChunk[NEXT_INDEX], 0);
            CacheBypassWriteInt(&pNextChunk[SIZE_INDEX], 0);
	}
	else
	{
		// resuse
		CacheBypassWriteInt(&pChunk[SIZE_INDEX], -prevSize);
                //printf("MALLOC: reusing space %d for size %d\n", prevSize, size);
	}

	//printf("MALLOC: chunk %p size %d\n", pChunk, pChunk[SIZE_INDEX]);
        //mpi_dump_mem();
        
	return (char*)&pChunk[DATA_INDEX];
}

/**
 */
void mpi_free_locked(char* p)
{
    
    
	if (p == NULL)
	{
            //printf("FREE: deleting NULL\n");
            return;
	}

	int* pChunk = &((int*) p)[-DATA_INDEX];

        int nextPrev = pChunk[NEXT_INDEX];
        int sizePrev = -pChunk[SIZE_INDEX];
	pChunk[SIZE_INDEX] = sizePrev;

        int* pNextChunk = &pChunk[nextPrev];
        
        int nextNext = pNextChunk[NEXT_INDEX];
        int sizeNext = pNextChunk[SIZE_INDEX];
        
        //printf("FREE: %p size %d sizeNext %d nextNext %d\n", p, sizePrev, sizeNext, nextNext);
        
        if (sizeNext == 0 && nextNext == 0)
        {
            pChunk[SIZE_INDEX] = 0;
            pChunk[NEXT_INDEX] = 0;
            
            //printf("FREE: merge with last entry\n");
        }
        if (sizeNext < 0)
        {
            // two consecutive free regions
            pChunk[SIZE_INDEX] = (sizePrev + sizeNext-2);
            pChunk[NEXT_INDEX] = nextNext+ nextPrev;
            
            //printf("FREE: merge next[next] addr = %p new current[next] addr = %p\n", &pNextChunk[pNextChunk[NEXT_INDEX]], &pChunk[pChunk[NEXT_INDEX]]);
        }
        
        //mpi_dump_mem();
//	printf("delete chunk %p\n", pChunk);
}


void* mpi_malloc(int size)
{
	void* pReturn;
	MPI_enterCriticalSection();
	pReturn = mpi_malloc_locked(size);
	MPI_leaveCriticalSection();
	return pReturn;
}

void* mpi_calloc(int size, int num)
{
	int total = size*num;
	void* pReturn = mpi_malloc(total);
	memset(pReturn, 0, total);
	return pReturn;
}

void mpi_free(void* p)
{
	MPI_enterCriticalSection();
	mpi_free_locked(p);
	MPI_leaveCriticalSection();
}

/**
 *
 */
/*
void* mpi_malloc_locked(int size)
{
	int* pChunk;
	int allocated;
	void* pReturn;

	MPI_Buffer_Count = CacheBypassReadInt(&MPI_Buffer_Count);

	pChunk = (int*) &MPI_Buffer[MPI_Buffer_Count];
	allocated = ((size + 3) / 4);
	allocated *= 4;
	allocated += 4;	// space for chunk size

	CacheBypassWriteInt(pChunk, allocated);
	pReturn = &pChunk[1];

	MPI_Buffer_Count += allocated;

	CacheBypassWriteInt(&MPI_Buffer_Count, MPI_Buffer_Count);

	return pReturn;
}



void mpi_free_locked(void* p)
{
	int* pChunk = (int*) ((int)p - 4);
	int allocated = CacheBypassReadInt(pChunk);
	void* pNext = (void*) (((int)pChunk) + allocated);
	MPI_Buffer_Count = CacheBypassReadInt(&MPI_Buffer_Count);
	int tailSize = ((int) &MPI_Buffer[MPI_Buffer_Count]) - (int) pNext;

	/ *CacheBypassBothMemcpy(pChunk, pNext, tailSize);

	MPI_Buffer_Count -= allocated;

	CacheBypassWriteInt(&MPI_Buffer_Count, MPI_Buffer_Count);* /
}*/

int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
	int rank;

	MPI_Msg* pMsg = NULL;
	int datasize;
	void* data;

//	MPI_ENTER_TRACE(MPI_Send);

	rank = mpi_rank();

	MPI_enterCriticalSection();

//	if (rank == 0) printf("[CPU%d] MPI_Send %d -> %d\n", rank, rank, dest);

	CacheBypassReadInt(&MPI_Message_Count);

	if (MPI_Message_Count >= NUM_OF_MPI_MESSAGES)
	{
		if (rank == 0) printf("[CPU%d] ERROR: too many msgs (%d) in queue", rank, MPI_Message_Count);
		else MPI_AddDebugLine('Q', MPI_Message_Count);

		MPI_leaveCriticalSection();

		return MPI_ERR_OVERFLOW;
	}

	pMsg = &MPI_Messages[MPI_Message_Count];

	MPI_Type_size(datatype, &datasize);


	//data = malloc(datasize * count);
	data = mpi_malloc_locked(datasize * count);

	if (data == NULL)
	{
		if (rank == 0) printf("[CPU%d] ERROR: failed to allocate MPI_Send buffer\n", rank);
		else MPI_AddDebugLine('M', rank);
		MPI_leaveCriticalSection();

		return MPI_ERR_MEMORY;
	}

	CacheBypassWriteInt(&MPI_Message_Count, CacheBypassReadInt(&MPI_Message_Count)+1);

	CacheBypassWriteMemcpy(data, buf, datasize * count);

	CacheBypassWriteInt((int*)&pMsg->data, (int) data);
	CacheBypassWriteInt((int*)&pMsg->dst , dest);
	CacheBypassWriteInt((int*)&pMsg->src , rank);
	CacheBypassWriteInt((int*)&pMsg->tag , tag);
	CacheBypassWriteInt((int*)&pMsg->type , datatype);
	CacheBypassWriteInt((int*)&pMsg->count , count);
	// pMsg->comm = comm;

	MPI_leaveCriticalSection();

//        printf("message sent %d->%d\n", rank, dest);
        
	return MPI_SUCCESS;
}

/**
 * Internal receive function
 */
int mpi_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status* status)
{
	int rank = mpi_rank();
	int i;
	int datasize;

	MPI_Msg* pTest = NULL;
	void* data = NULL;

	MPI_enterCriticalSection();

//	if (rank == 0)	printf("[CPU%d] mpi_Irecv %d -> %d count:%d tag:%d data:%p\n", rank, source, rank, count, tag, buf);
//	if (rank == 0) MPI_Debug_Report();

	CacheBypassReadInt(&MPI_Message_Count);

	// find a message matching criteria
	for (i=0; i < MPI_Message_Count; i++)
	{
		pTest = &MPI_Messages[i];

		CacheBypassRead((int*) pTest, sizeof(MPI_Msg));

		if ((pTest->dst != rank))
			continue;

		if ((pTest->src != source) && (source != MPI_ANY_SOURCE))
			continue;

		if (tag == MPI_ANY_TAG)
		{
			if ((pTest->tag & MPI_TAG_INTERNAL_MASK) != 0)
				continue;
		}
		else
		{
			if (pTest->tag != tag)
				continue;
		}

		if (pTest->type != datatype)
			continue;

		if (pTest->count != count)
			continue;

		// if (pTest->comm != comm)
		//continue;

		data = pTest->data;

		if (status != NULL)
		{
			status->MPI_SOURCE = pTest->src;
			status->MPI_TAG = pTest->tag;
		}

		// swap last message with removed message
		if (i == (MPI_Message_Count -1))
		{
			// skip swap for last element
		}
		else
		{
			MPI_Msg* pSwap = &MPI_Messages[MPI_Message_Count-1];

			CacheBypassBothMemcpy((int*) pTest, (int*) pSwap, sizeof(MPI_Msg));
		}


		CacheBypassWriteInt(&MPI_Message_Count, MPI_Message_Count-1);

		break;
	}



	if (data == NULL)
	{
		MPI_leaveCriticalSection();
		return MPI_ERR_PENDING;
	}

//	if (rank == 0) printf("\trcv data: %p\n", data);

	MPI_Type_size(datatype, &datasize);

	CacheBypassReadMemcpy(buf, data, datasize * count);

	//free(data);
	mpi_free_locked(data);

	MPI_leaveCriticalSection();

	return MPI_SUCCESS;
}


void MPI_Debug_Show_Messages()
{
	int rank = mpi_rank();
	int i,j;

	if (rank != 0)
		return;


	CacheBypassReadInt(&MPI_Message_Count);

	printf("MPI_Message_Count: %d\n" , MPI_Message_Count);

	for (i=0; i < MPI_Message_Count; i++)
	{
		MPI_Msg* pTest = &MPI_Messages[i];

		CacheBypassRead((int*) pTest, sizeof(MPI_Msg));

		printf("   Msg[%d] %d -> %d count:%d tag:%d data:%p {", i, pTest->src, pTest->dst, pTest->count, pTest->tag, pTest->data);
                
                for (j=0; j < pTest->count; j++)
                {
                    if (pTest->type == MPI_INT)
                    {
                        int* pArray = (int*)pTest->data;
                        printf("%d", pArray[j]);
                        if (j < pTest->count -1)
                            printf(",");
                    }
                }
                printf("}\n");
	}
}



/**
 * Used to reset all the MPI environment vars, usually during system reset
 */
void mpi_clear_vars()
{
	CacheBypassWriteInt(&MPI_Buffer[NEXT_INDEX], 0);
	CacheBypassWriteInt(&MPI_Buffer[SIZE_INDEX], 0);
	CacheBypassWriteInt(&MPI_Message_Count, 0);
	
        MPI_DebugInit();
}

int mpi_dump_mem()
{
	int* pChunk = MPI_Buffer;

        printf("mem dump >>>>>>>\n");
        
	do
	{
		int index = pChunk - MPI_Buffer;
		int nexIndex = pChunk[NEXT_INDEX];
		int size = pChunk[SIZE_INDEX];

		printf("index [%d] address [%p] next-index [%d] size[%d] \n", index, pChunk, nexIndex, size);
	
		if (nexIndex == 0)
		{
			pChunk = NULL;
		}
		else if (nexIndex < MPI_BUFFER_SIZE)
		{
			pChunk = &pChunk[nexIndex];
		}
		else
			pChunk = NULL;
	} while (pChunk != NULL);
        
        printf("mem dump <<<<<<<<\n");

}