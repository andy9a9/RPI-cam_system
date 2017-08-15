#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <pthread.h>

// TODO: only for testing, change to -1
#define THREAD_ISR_TIMEOUT 5

extern pthread_mutex_t *pSctMutex;

int InitIsr(char *pFile, void (*pCallbackFunc)(int));
void CleanIsrThread(void);
void *IsrThread(void *args);

#endif // INTERRUPTS_H
