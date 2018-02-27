/**
 * 
 * Copyright (C) 2015, David Castells-Rufas <david.castells@uab.es>, CEPHIS, Universitat Autonoma de Barcelona  
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

#include "cpuid.h"
#include "../../config.h"

#include <stdio.h>
#include <pthread.h>

static int creatingThreads = 1;
pthread_t mpi_threads[NUM_OF_PROCESSORS];

void  ThreadProc ( void *ptr );


int getCpuIdentifier()
{
    // busy wait until all threads are created
    while (creatingThreads);
    
    int i;
    for (i = 0; i < NUM_OF_PROCESSORS; i++)
    {
        if (mpi_threads[i] == pthread_self())
            return i;
    }
    
    printf("Thread %p NOT FOUND\n", pthread_self());
    for (i = 0; i < NUM_OF_PROCESSORS; i++)
    {
        printf("Tread[%d] = %p\n", i, mpi_threads[i]);
    }
    
    return -1;
}

void createSlaveThreads(main_func mf)
{
    int i;
    
    creatingThreads = 1;
    mpi_threads[0] = pthread_self();
    
    for (i = 1; i < NUM_OF_PROCESSORS; i++)
    {
        pthread_t pth;
        
        pthread_create(&pth, NULL, (void*) ThreadProc, (void*) mf);

        mpi_threads[i] = pth;

        printf("Slave %d has handle %p\n", i, mpi_threads[i]);
    }
    
    creatingThreads = 0;
}


void ThreadProc (void* lpdwThreadParam ) 
{
    main_func mf = (main_func) lpdwThreadParam;
    
    printf("Going to invoke main from thread %p\n", pthread_self());
    
    (*mf)(0, NULL);
}