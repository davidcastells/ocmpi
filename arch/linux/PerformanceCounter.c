//#include <windows.h>

#include "PerformanceCounter.h"

#include <sys/time.h>

double freq;




void perfCounter(uint64 * p64)
{
    struct timeval time_val;
    gettimeofday(&time_val, 0);

    double secs = (double) time_val.tv_sec;

    secs += ((double) time_val.tv_usec) / 1e6;

    *p64 = (uint64) (secs * 1e6); 
}

double secondsBetweenLaps(uint64 t0, uint64 tf)
{
    uint64 diff = tf - t0;
    
    double secs = diff / 1e6;
    
    return secs;
}