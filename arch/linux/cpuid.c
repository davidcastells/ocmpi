
#include "cpuid.h"
#include "../../config.h"

#include <stdio.h>
#include <pthread.h>

pthread_t mpi_threads[NUM_OF_PROCESSORS];

void  ThreadProc ( void *ptr );


int getCpuIdentifier()
{
    int i;
    for (i = 0; i < NUM_OF_PROCESSORS; i++)
    {
        if (mpi_threads[i] == pthread_self())
            return i;
    }
    
    return -1;
}

void createSlaveThreads(main_func mf)
{
    int i;
    
    mpi_threads[0] = pthread_self();
    
    for (i = 1; i < NUM_OF_PROCESSORS; i++)
    {
        pthread_t pth;
        
        pthread_create(&pth, NULL, (void*) ThreadProc, (void*) mf);

        mpi_threads[i] = pth;

        printf("Slave %d has handle %d\n", i, (int) mpi_threads[i]);
    }
    
}


void ThreadProc (void* lpdwThreadParam ) 
{
    main_func mf = (main_func) lpdwThreadParam;
    
    printf("Going to invoke main from thread %d\n", (int) pthread_self());
    
    (*mf)(0, NULL);
}