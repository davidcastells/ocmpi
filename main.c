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
 * File:   main.c
 * Author: dcr
 *
 * Created on 17 de diciembre de 2014, 20:17
 */

#include <stdio.h>
#include <stdlib.h>

#include "examples/mpiqueens2.h"
#include "examples/mpi_pi.h"

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
    
    createSlaveThreads(mpi_pi_montecarlo_int_main);
    return mpi_pi_montecarlo_int_main(argc, argv);
    
    /*
    createSlaveThreads(mpiqueens2_main);
    return mpiqueens2_main(argc, argv);
     * /
    /*
    createSlaveThreads(jms_nqueens_main);
    return jms_nqueens_main(argc, argv);*/
}

