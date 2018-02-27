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
 * mpi.h
 *
 *  Created on: 12/11/2013
 *      Author: test64
 */

#ifndef MPI_H_
#define MPI_H_

#define USING_OC_MPI		1

#define MPI_Comm	int
#define MPI_Op          int

/* error return classes */
#define MPI_SUCCESS          0      /* Successful return code */
#define MPI_ERR_BUFFER       1      /* Invalid buffer pointer */
#define MPI_ERR_COUNT        2      /* Invalid count argument */
#define MPI_ERR_TYPE         3      /* Invalid datatype argument */
#define MPI_ERR_TAG          4      /* Invalid tag argument */
#define MPI_ERR_COMM         5      /* Invalid communicator */
#define MPI_ERR_RANK         6      /* Invalid rank */
#define MPI_ERR_ROOT         7      /* Invalid root */
#define MPI_ERR_GROUP        8      /* Null group passed to function */
#define MPI_ERR_OP           9      /* Invalid operation */
#define MPI_ERR_TOPOLOGY    10      /* Invalid topology */
#define MPI_ERR_DIMS        11      /* Illegal dimension argument */
#define MPI_ERR_ARG         12      /* Invalid argument */
#define MPI_ERR_UNKNOWN     13      /* Unknown error */
#define MPI_ERR_TRUNCATE    14      /* message truncated on receive */
#define MPI_ERR_OTHER       15      /* Other error; use Error_string */
#define MPI_ERR_INTERN      16      /* internal error code    */
#define MPI_ERR_IN_STATUS   17      /* Look in status for error value */
#define MPI_ERR_PENDING     18      /* Pending request */
#define MPI_ERR_REQUEST     19      /* illegal mpi_request handle */

#define MPI_ERR_TIMEOUT		20		/* defined by dcr to support timeouts */
#define MPI_ERR_LASTCODE    (256*16+18)      /* Last error code*/

// error codes
#define MPI_ERR_MEMORY			20		// could not allocate memory
#define MPI_ERR_OVERFLOW		21		// too many messsages


#define MPI_COMM_WORLD	0


#define MPI_Datatype	int

#define MPI_CHAR           (1)
#define MPI_UNSIGNED_CHAR  (2)
#define MPI_BYTE           (3)
#define MPI_SHORT          (4)
#define MPI_UNSIGNED_SHORT (5)
#define MPI_INT            (6)
#define MPI_UNSIGNED       (7)
#define MPI_LONG           (8)
#define MPI_UNSIGNED_LONG  (9)
#define MPI_FLOAT          (10)
#define MPI_DOUBLE         (11)
#define MPI_LONG_DOUBLE    (12)

#define MPI_ANY_SOURCE      -1

#define MPI_TAG_UB         		0x0000FFFF
#define MPI_TAG_INTERNAL_MASK	0xFFFF0000
#define MPI_ANY_TAG         		(MPI_TAG_UB+2)
#define MPI_INTERNAL_BARRIER_TAG	(MPI_TAG_UB+3)
#define MPI_INTERNAL_REDUCE_TAG         (MPI_TAG_UB+4)

#define MPI_REQUEST_RECV	1
#define MPI_REQUEST_SEND	2


#define MPI_MAX     1
#define MPI_MIN     2
#define MPI_SUM     3 
#define MPI_PROD    4
#define MPI_LAND    5
#define MPI_BAND    6
#define MPI_LOR     7
#define MPI_BOR     8
#define MPI_LXOR    9
#define MPI_BXOR    10
#define MPI_MAXLOC  11
#define MPI_MINLOC  12

/*
*/
typedef struct MPI_Status
{
  int MPI_SOURCE;
  int MPI_TAG;
  int MPI_ERROR;
  int _countrecv;
} MPI_Status;

typedef struct _MPI_Msg
{
	int src;
	int dst;
	MPI_Datatype type;
	int tag;
	int comm;
	int count;
	void* data;
} MPI_Msg;

typedef struct _MPI_Request
{
	int src;
	int dst;
	void* data;
	int count;
	MPI_Datatype type;
	int tag;
	int comm;
} MPI_Request;

int MPI_Init(int* argc, char*** args);
int MPI_Finalize();
int MPI_Comm_rank(MPI_Comm comm, int *rank);
int MPI_Comm_size(MPI_Comm comm,  int *size);
double MPI_Wtime();
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Request* request);
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
int MPI_Test(MPI_Request* request, int* flag, MPI_Status* status);
int MPI_Type_size(MPI_Datatype datatype, int *size);
int MPI_Wait(MPI_Request *request, MPI_Status *status);
int MPI_Barrier( MPI_Comm comm );

// private
void MPI_enterCriticalSection();
void MPI_leaveCriticalSection();

void MPI_AddDebugLine(char code, int v);


int mpi_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status* status);

void* mpi_calloc(int size, int num);

int mpi_dump_mem();

void mpi_clear_vars();
int mpi_rank();
void mpi_sleepMilliseconds(int milliseconds);

//extern char* MPI_Debug;
extern int mpi_global_error;
extern double MPI_Tx_Timeout;

void MPI_Debug_Show_Messages();
void MPI_DebugInit();

#endif /* MPI_H_ */
