#include "mpi.h"
#include "config.h"

#include <stdio.h>

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

// [src][dst]
MPI_Msg mailslots[NUM_OF_PROCESSORS][NUM_OF_PROCESSORS];


void MPI_Debug_Show_Messages()
{
    int src, dst;
    
    printf("Messages >>>>>>>>\n");
    for (src=0; src < NUM_OF_PROCESSORS; src++)
        for (dst=0; dst < NUM_OF_PROCESSORS; dst++)
        {
            MPI_Msg* pMsg = &mailslots[src][dst];
            
            if (pMsg->data != NULL)
            {
                printf("Message[%d][%d] %d -> %d %p\n", src, dst, pMsg->src, pMsg->dst, pMsg->data);
            }
        }
    
        printf("Messages <<<<<<<<\n");

}

int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
    MPI_Msg* pMsg = &mailslots[mpi_rank()][dest];
    
#ifdef WIN32
//    printf("message sent %d->%d\n", mpi_rank(), dest);
#endif
    
    // wait for data zero (is the indication that mailslot is free)
    while (pMsg->data != NULL);
    
    // put the message
    pMsg->count = count;
    pMsg->type = datatype;
    pMsg->comm = comm;
    pMsg->dst = dest;
    pMsg->src = mpi_rank();
    pMsg->tag = tag;
    
    pMsg->data = buf;
    
    // wait for data zero (means that the message has been consumed)
    while (pMsg->data != NULL);
}


int mpi_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status* status)
{
    if (source == MPI_ANY_SOURCE)
    {
        int datasize;
        int src;
        for (src = 0; src < NUM_OF_PROCESSORS; src++)
        {
            MPI_Msg* pMsg = &mailslots[src][mpi_rank()];

//            printf("mpi_Irecv: checking msg %d -> %d %p\n", pMsg->src, pMsg->dst, pMsg->data);
            
            if (pMsg->data == NULL)
                continue;
            
            if (pMsg->type != datatype)
            {   
//                printf("Different datatype\n"); 
                continue;
            }
            
            if (tag == MPI_ANY_TAG)
            {
                if ((pMsg->tag & MPI_TAG_INTERNAL_MASK) == 0)
                {
//                    printf("Internal message\n");
                    continue;
                }
            }
            else if (pMsg->tag != tag)
            {   
//                printf("different tag\n");
                continue; 
            }

            if (pMsg->comm != comm)
            {
//                printf("differnt comm\n");
                continue;
            }
            
            if (pMsg->count != count)
            {
//                printf("different count");
                continue;
            }
            
            MPI_Type_size(datatype, &datasize);

            CacheBypassReadMemcpy(buf, pMsg->data, datasize * count);

            if (status != NULL)
            {
		status->MPI_SOURCE = pMsg->src;
		status->MPI_TAG = pMsg->tag;
            }
            
            // signal that we consumed the message
            pMsg->data = NULL;
            
            return MPI_SUCCESS;
        }
        
        return MPI_ERR_PENDING;
    }
    else
    {
        int datasize;
        MPI_Msg* pMsg = &mailslots[source][mpi_rank()];
        
        // wait for data non zero (is the indication that mailslot is full)
        if (pMsg->data == NULL)
            return MPI_ERR_PENDING;

        if (pMsg->type != datatype)
            return MPI_ERR_TYPE;
            
        if (tag == MPI_ANY_TAG)
        {
            if ((pMsg->tag & MPI_TAG_INTERNAL_MASK) == 0)
                return MPI_ERR_TAG;
        }
        else if (pMsg->tag != tag)
            return MPI_ERR_TAG;
            
        if (pMsg->comm != comm)
            return MPI_ERR_COMM;
            
        if (pMsg->count != count)
            return MPI_ERR_COUNT;
            
        MPI_Type_size(datatype, &datasize);

        CacheBypassReadMemcpy(buf, pMsg->data, datasize * count);
        
        // signal that we consumed the message
        pMsg->data = NULL;
        
        return MPI_SUCCESS;
    }
}

void mpi_clear_vars()
{
}