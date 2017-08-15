#ifndef INTERRUPTS_H
#define INTERRUPTS_H

#include <pthread.h>

// wait until change
#define THREAD_ISR_TIMEOUT -1

extern pthread_mutex_t *pSctMutex;

int InitIsr(char *pFile, void (*pCallbackFunc)(int));
int WaitIsr(void);
void CleanIsrThread(void);
void *IsrThread(void *args);

#endif // INTERRUPTS_H
