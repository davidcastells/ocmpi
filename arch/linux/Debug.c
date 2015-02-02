#include "CacheBypass.h"
#include "Debug.h"
#include "../../mpi.h"

#include <stdio.h>

char MPI_Debug[0x400*4];
int MPI_DebugCount = 0;

void MPI_DebugInit()
{
    CacheBypassWriteInt(&MPI_DebugCount, 0);
}

void MPI_AddDebugLine(char code, int v)
{
    printf("%d%c%d\n", mpi_rank(), code, v);
}

/**
 * @deprecated use MPI_AddDebugLine
 */
void MPI_AddDebug(char c)
{
    printf("%c", c);
}

/**
 *
 */
int MPI_DumpDebug()
{
    return 0;
}
