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
#include <sys/alt_cache.h>
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


/**
 * In this buffering strategy we assume that most of the communication will happen
 * between master and slaves. So we get rid of slave to slave mailboxes.
 * However we allow slave communication, but just with one mailbox.
 */

// [type][n]
MPI_Msg mailslots[3][NUM_OF_PROCESSORS];

#define MAILSLOT_TYPE_MASTER_TO_SLAVE   0   // mailslots[0][n] are used to store the messages from 0->n
#define MAILSLOT_TYPE_SLAVE_TO_MASTER   1   // mailslots[1][n] are used to store the messages from n->0
#define MAILSLOT_TYPE_SLAVE_TO_SLAVE    2   // mailslots[2][n] are used to store the messages from ?->n

void MPI_Debug_Show_Messages()
{
    int type, n;
    
    printf("Messages >>>>>>>>\n");
    for (type=MAILSLOT_TYPE_MASTER_TO_SLAVE; type <= MAILSLOT_TYPE_SLAVE_TO_SLAVE; type++)
        for (n=0; n < NUM_OF_PROCESSORS; n++)
        {
            MPI_Msg* pMsg = &mailslots[type][n];
            
            if (CacheBypassReadInt((int*)&pMsg->data) != NULL)
            {
                printf("Message[%d][%d] %d -> %d *%d #%d %p\n", type, n,
                		CacheBypassReadInt(&pMsg->src),
                		CacheBypassReadInt(&pMsg->dst),
                		CacheBypassReadInt(&pMsg->count),
                		CacheBypassReadInt(&pMsg->tag),
                		CacheBypassReadInt((int*)&pMsg->data));
            }
        }
    
        printf("Messages <<<<<<<<\n");

}

int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    int rank;
    int datasize;
    MPI_Msg* pMsg;
    uint64 t0, tf;
    
    rank = mpi_rank();
    
    if (rank == 0)
        pMsg = &mailslots[MAILSLOT_TYPE_MASTER_TO_SLAVE][dest];
    else if (dest == 0)
        pMsg = &mailslots[MAILSLOT_TYPE_SLAVE_TO_MASTER][rank];
    else
        pMsg = &mailslots[MAILSLOT_TYPE_SLAVE_TO_SLAVE][dest];


#ifdef WIN32
//    printf("message sent %d->%d\n", mpi_rank(), dest);
#endif
    
    // wait for data zero (is the indication that mailslot is free)
    perfCounter(&t0);
    while (CacheBypassReadInt(&pMsg->data) != NULL)
    {
    	perfCounter(&tf);
    	if (secondsBetweenLaps(t0, tf) > MPI_Tx_Timeout)
    	{
    		CacheBypassWriteInt(&mpi_global_error, 1);
    		return MPI_ERR_TIMEOUT;
    	}
    }
    
    // put the message
    CacheBypassWriteInt(&pMsg->count, count);
    CacheBypassWriteInt(&pMsg->type, datatype);
    CacheBypassWriteInt(&pMsg->comm, comm);
    CacheBypassWriteInt(&pMsg->dst, dest);
    CacheBypassWriteInt(&pMsg->src, rank);
    CacheBypassWriteInt(&pMsg->tag, tag);
    
    MPI_Type_size(datatype, &datasize);

#ifdef NIOS
    alt_dcache_flush(buf, datasize * count);
#endif
    
    CacheBypassWriteInt(&pMsg->data, (int) buf);
    
#ifdef NIOS
    MPI_enterCriticalSection();
    printf("cpu[%d] message sent %d->%d p:%p %p\n", rank, mpi_rank(), dest, pMsg, CacheBypassReadInt(&pMsg->data));
    MPI_leaveCriticalSection();
#endif

    // wait for data zero (means that the message has been consumed)
    while (CacheBypassReadInt(&pMsg->data) != NULL);
}


/**
 * 
 * @param buf
 * @param count
 * @param datatype
 * @param source
 * @param tag
 * @param comm
 * @param status
 * @return 
 */
int mpi_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status* status)
{
	int rank = mpi_rank();

    if ((source == MPI_ANY_SOURCE) && (rank == 0))
    {
        int datasize;
        int src;
        for (src = 0; src < SIZE_OF_MPI_APPLICATION; src++)
        {
            MPI_Msg* pMsg = &mailslots[MAILSLOT_TYPE_SLAVE_TO_MASTER][src];

   /*         MPI_enterCriticalSection();
            printf("mpi_Irecv: checking msg %d -> %d p:%p %p\n", src, rank, pMsg, CacheBypassReadInt(&pMsg->data));
            MPI_leaveCriticalSection();*/
            
            if (CacheBypassReadInt(&pMsg->data) == NULL)
                continue;
            
            if (CacheBypassReadInt(&pMsg->type) != datatype)
            {   
//                printf("Different datatype\n"); 
                continue;
            }
            
            if (tag == MPI_ANY_TAG)
            {
                if ((CacheBypassReadInt(&pMsg->tag) & MPI_TAG_INTERNAL_MASK) == 0)
                {
//                    printf("Internal message\n");
                    continue;
                }
            }
            else if (CacheBypassReadInt(&pMsg->tag) != tag)
            {   
//                printf("different tag\n");
                continue; 
            }

/*            if (CacheBypassReadInt(&pMsg->comm) != comm)
            {
//                printf("differnt comm\n");
                continue;
            }*/
            
            if (CacheBypassReadInt(&pMsg->count) != count)
            {
//                printf("different count");
                continue;
            }
            
            MPI_Type_size(datatype, &datasize);

            CacheBypassBothMemcpy(buf, CacheBypassReadInt(&pMsg->data), datasize * count);

            if (status != NULL)
            {
            	status->MPI_SOURCE = src; // CacheBypassReadInt(&pMsg->src);
            	status->MPI_TAG = CacheBypassReadInt(&pMsg->tag);
            }
            
            MPI_enterCriticalSection();
            printf("cpu[%d] any source received %d -> %d\n", rank, src, rank );
            MPI_leaveCriticalSection();
            // signal that we consumed the message
            CacheBypassWriteInt(&pMsg->data , NULL);
            
            return MPI_SUCCESS;
        }
        
        return MPI_ERR_PENDING;
    }
    else if ((source == MPI_ANY_SOURCE) && (rank != 0))
    {
        int datasize;
        int src;
        for (src = MAILSLOT_TYPE_SLAVE_TO_MASTER; src <= MAILSLOT_TYPE_SLAVE_TO_SLAVE; src++)
        {
            MPI_Msg* pMsg = &mailslots[src][rank];

   /*         MPI_enterCriticalSection();
            printf("mpi_Irecv: checking msg %d -> %d p:%p %p\n", src, rank, pMsg, CacheBypassReadInt(&pMsg->data));
            MPI_leaveCriticalSection();*/
            
            if (CacheBypassReadInt(&pMsg->data) == NULL)
                continue;
            
            if (CacheBypassReadInt(&pMsg->type) != datatype)
            {   
//                printf("Different datatype\n"); 
                continue;
            }
            
            if (tag == MPI_ANY_TAG)
            {
                if ((CacheBypassReadInt(&pMsg->tag) & MPI_TAG_INTERNAL_MASK) == 0)
                {
//                    printf("Internal message\n");
                    continue;
                }
            }
            else if (CacheBypassReadInt(&pMsg->tag) != tag)
            {   
//                printf("different tag\n");
                continue; 
            }

/*            if (CacheBypassReadInt(&pMsg->comm) != comm)
            {
//                printf("differnt comm\n");
                continue;
            }*/
            
            if (CacheBypassReadInt(&pMsg->count) != count)
            {
//                printf("different count");
                continue;
            }
            
            MPI_Type_size(datatype, &datasize);

            CacheBypassBothMemcpy(buf, CacheBypassReadInt(&pMsg->data), datasize * count);

            if (status != NULL)
            {
            	status->MPI_SOURCE = src; // CacheBypassReadInt(&pMsg->src);
            	status->MPI_TAG = CacheBypassReadInt(&pMsg->tag);
            }
            
            MPI_enterCriticalSection();
            printf("cpu[%d] any source received %d -> %d\n", rank, src, rank );
            MPI_leaveCriticalSection();
            // signal that we consumed the message
            CacheBypassWriteInt(&pMsg->data , NULL);
            
            return MPI_SUCCESS;
        }
        
        return MPI_ERR_PENDING;
    }
    else if (source == 0)
    {
        int datasize;
        MPI_Msg* pMsg = &mailslots[MAILSLOT_TYPE_MASTER_TO_SLAVE][mpi_rank()];
        
        // wait for data non zero (is the indication that mailslot is full)
        if (CacheBypassReadInt(&pMsg->data) == NULL)
            return MPI_ERR_PENDING;

        if (CacheBypassReadInt(&pMsg->type) != datatype)
            return MPI_ERR_TYPE;
            
        if (tag == MPI_ANY_TAG)
        {
            if ((CacheBypassReadInt(&pMsg->tag) & MPI_TAG_INTERNAL_MASK) == 0)
                return MPI_ERR_TAG;
        }
        else if (CacheBypassReadInt(&pMsg->tag) != tag)
            return MPI_ERR_TAG;
            
        if (CacheBypassReadInt(&pMsg->comm) != comm)
            return MPI_ERR_COMM;
            
        if (CacheBypassReadInt(&pMsg->count) != count)
            return MPI_ERR_COUNT;
            
        MPI_Type_size(datatype, &datasize);

        CacheBypassReadMemcpy(buf, pMsg->data, datasize * count);
        
        // signal that we consumed the message
        CacheBypassWriteInt(&pMsg->data , NULL);
        
        return MPI_SUCCESS;
    }
    else if (rank == 0)
    {
        int datasize;
        MPI_Msg* pMsg = &mailslots[MAILSLOT_TYPE_SLAVE_TO_MASTER][source];
        
        // wait for data non zero (is the indication that mailslot is full)
        if (CacheBypassReadInt(&pMsg->data) == NULL)
            return MPI_ERR_PENDING;

        if (CacheBypassReadInt(&pMsg->type) != datatype)
            return MPI_ERR_TYPE;
            
        if (tag == MPI_ANY_TAG)
        {
            if ((CacheBypassReadInt(&pMsg->tag) & MPI_TAG_INTERNAL_MASK) == 0)
                return MPI_ERR_TAG;
        }
        else if (CacheBypassReadInt(&pMsg->tag) != tag)
            return MPI_ERR_TAG;
            
        if (CacheBypassReadInt(&pMsg->comm) != comm)
            return MPI_ERR_COMM;
            
        if (CacheBypassReadInt(&pMsg->count) != count)
            return MPI_ERR_COUNT;
            
        MPI_Type_size(datatype, &datasize);

        CacheBypassReadMemcpy(buf, pMsg->data, datasize * count);
        
        // signal that we consumed the message
        CacheBypassWriteInt(&pMsg->data , NULL);
        
        return MPI_SUCCESS;
    }
    else
    {
        int datasize;
        MPI_Msg* pMsg = &mailslots[MAILSLOT_TYPE_SLAVE_TO_SLAVE][mpi_rank()];
        
        if (CacheBypassReadInt(&pMsg->src) != source)
            return MPI_ERR_PENDING;
        
        // wait for data non zero (is the indication that mailslot is full)
        if (CacheBypassReadInt(&pMsg->data) == NULL)
            return MPI_ERR_PENDING;

        if (CacheBypassReadInt(&pMsg->type) != datatype)
            return MPI_ERR_TYPE;
            
        if (tag == MPI_ANY_TAG)
        {
            if ((CacheBypassReadInt(&pMsg->tag) & MPI_TAG_INTERNAL_MASK) == 0)
                return MPI_ERR_TAG;
        }
        else if (CacheBypassReadInt(&pMsg->tag) != tag)
            return MPI_ERR_TAG;
            
        if (CacheBypassReadInt(&pMsg->comm) != comm)
            return MPI_ERR_COMM;
            
        if (CacheBypassReadInt(&pMsg->count) != count)
            return MPI_ERR_COUNT;
            
        MPI_Type_size(datatype, &datasize);

        CacheBypassReadMemcpy(buf, pMsg->data, datasize * count);
        
        // signal that we consumed the message
        CacheBypassWriteInt(&pMsg->data , NULL);
        
        return MPI_SUCCESS;
    }
}

void mpi_clear_vars()
{
}
