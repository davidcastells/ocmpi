#include <windows.h>

#include "PerformanceCounter.h"

double freq;
int64 start;

void sec_init(void)
{
	LARGE_INTEGER lFreq, lCnt;
	QueryPerformanceFrequency(&lFreq);
	freq = (double)lFreq.LowPart;
	QueryPerformanceCounter(&lCnt);
	start = lCnt.LowPart;
}
/* return number of seconds since sec_init was called with
** a gross amount of detail
*/
double sec(void)
{
	LARGE_INTEGER lCnt;
	int64 tcnt;
	QueryPerformanceCounter(&lCnt);
 	tcnt = lCnt.LowPart - start;
 	return ((double)tcnt) / freq;
}


void perfCounter(uint64 * p64)
{
    LARGE_INTEGER lCnt;
    QueryPerformanceCounter(&lCnt);
    *p64 = lCnt.QuadPart;
}

double secondsBetweenLaps(uint64 t0, uint64 tf)
{
    uint64 diff = tf - t0;
    LARGE_INTEGER lFreq;
    QueryPerformanceFrequency(&lFreq);
    
    double secs = (double) diff / (double) lFreq.QuadPart;
    
    return secs;
}