#include <windows.h>

CRITICAL_SECTION critSec;


int initMutex()
{
    InitializeCriticalSection(&critSec);
    return 0;
}

void mutexAcquire()
{
    EnterCriticalSection(&critSec);
    
}

void mutexRelease()
{
    LeaveCriticalSection(&critSec);    
}

int mutexState()
{
    return 0;
}