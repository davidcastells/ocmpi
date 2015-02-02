#include <windows.h>
#include "cpuid.h"
#include "../../config.h"

#include <stdio.h>

DWORD mpi_threads[NUM_OF_PROCESSORS];

DWORD ThreadProc (LPVOID lpdwThreadParam ) ;


int getCpuIdentifier()
{
    int i;
    for (i = 0; i < NUM_OF_PROCESSORS; i++)
    {
        if (mpi_threads[i] == GetCurrentThreadId())
            return i;
    }
    
    return -1;
}

void createSlaveThreads(main_func mf)
{
    int i;
    
    mpi_threads[0] = GetCurrentThreadId();
    
    for (i = 1; i < NUM_OF_PROCESSORS; i++)
    {
        DWORD tid;
        
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) ThreadProc, (LPVOID) mf, 0, &tid);

        mpi_threads[i] = tid;

        printf("Slave %d has handle %p\n", i, mpi_threads[i]);
    }
    
}


DWORD ThreadProc (LPVOID lpdwThreadParam ) 
{
    main_func mf = (main_func) lpdwThreadParam;
    
    printf("Going to invoke main from thread %p\n", GetCurrentThreadId());
    
    (*mf)(0, NULL);
}