/* 
 * File:   main.c
 * Author: dcr
 *
 * Created on 17 de diciembre de 2014, 20:17
 */

#include <stdio.h>
#include <stdlib.h>

#include "examples/mpiqueens2.h"

#ifdef WIN32
    #include "arch/windows/mutex.h"
    #include "arch/windows/cpuid.h"
#endif

#ifdef LINUX
    #include "arch/linux/mutex.h"
    #include "arch/linux/cpuid.h"
#endif

int jms_nqueens_main(int argc, char** argv);

/*
 * 
 */
int main(int argc, char** argv) 
{
    initMutex();
    
    /*createSlaveThreads(mpiqueens2_main);
    
    return mpiqueens2_main(argc, argv);*/
    
    createSlaveThreads(jms_nqueens_main);
    return jms_nqueens_main(argc, argv);
}

