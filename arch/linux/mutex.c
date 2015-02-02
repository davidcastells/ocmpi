
#include <pthread.h>

pthread_mutex_t  mutex;


int initMutex()
{
    
    pthread_mutex_init (& mutex , NULL );
    return 0;
}

void mutexAcquire()
{
     pthread_mutex_lock (&mutex );  
}

void mutexRelease()
{
    pthread_mutex_unlock (& mutex );   
}

int mutexState()
{
    return 0;
}