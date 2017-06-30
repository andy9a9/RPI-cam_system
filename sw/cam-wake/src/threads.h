#ifndef PTHREAD_H
#define PTHREAD_H

#include <pthread.h>

extern pthread_mutex_t *pSctMutex;

int InitIsrThread(void);

#endif // PTHREAD_H
