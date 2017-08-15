#include "common.h"
#include <time.h>

#include "threads.h"

pthread_mutex_t *pSctMutex = NULL;
pthread_cond_t sctCond;

int InitIsrThread(void) {
    int ret = 0;

    // allocate mutex
    pSctMutex = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));

    // init mutex
    if (pthread_mutex_init(pSctMutex, NULL)) {
        fprintf(stderr, "Error: can not create initialize mutex for scanning thread!\n");
        return -1;
    }

    // thread attributes
    pthread_condattr_t attr;

    // init thread attributes
    if (pthread_condattr_init(&attr)) {
        fprintf(stderr, "Error: can not initialize attributes for scanning thread!\n");
        ret = -2;
    } else if (pthread_condattr_setclock(&attr, CLOCK_MONOTONIC)) {
        fprintf(stderr, "Error: can not set clock attributes for scanning thread!\n");
        ret = -3;
    } else if (pthread_cond_init(&sctCond, &attr)) {
        fprintf(stderr, "Error: can not set conditions for scanning thread!\n");
        ret = -4;
    }

    // check return code
    if (ret <= -3)
        pthread_condattr_destroy(&attr);
    if (ret <= -2)
        pthread_mutex_destroy(pSctMutex);

    return ret;
}
