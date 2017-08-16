#include "common.h"
#include <time.h>
#include <poll.h>

#include "interrupts.h"

pthread_mutex_t *pSctMutex = NULL;
pthread_cond_t sctCond;

volatile
static volatile int watchFd = -1;
static void (*pIsrFunction)(int) = NULL;

int InitThread(void) {
    int ret = 0;

    // allocate mutex
    pSctMutex = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));

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

int InitIsr(char *pFile, void (*pCallbackFunc)(int)) {
    // check file
    if (pFile == NULL) {
        fprintf(stderr, "Error: can not watch empty filename!\n");
        return 1;
    }

    // open file for watching
    if ((watchFd = open(pFile, O_RDONLY)) < 0) {
        fprintf(stderr, "Error: can not open file %s, err:%s!\n", pFile, strerror(errno));
        watchFd = -1;
        return 3;
    }

    char c;
    // clear pending interrupt
    if (read(watchFd, &c, 1) != 1) {
        fprintf(stderr, "Error: can not read file %s, err:%s!\n", pFile, strerror(errno));
        watchFd = -1;
        return 4;
    }

    // check if thread is used
    if (pCallbackFunc != NULL) {
        // init thread
        if (InitThread() != 0) {
            watchFd = -1;
            pIsrFunction = NULL;
            return 5;
        }

        // save callback function
        pIsrFunction = pCallbackFunc;
    }

    return 0;
}

int WaitIsr(void) {
    int ret;
    int value = -1;

    struct pollfd stPoll;

    // check if thread has been initialized
    if (watchFd < 0) {
        fprintf(stderr, "Error: ISR has not been initialized!\n");
        return value;
    }

    // fill polling structure
    stPoll.fd = watchFd;
    stPoll.events = POLLPRI | POLLERR;

    // wait for interrupt
    ret = poll(&stPoll, 1, THREAD_ISR_TIMEOUT);
    // check response
    if (ret > 0) {
        // rewind
        lseek(watchFd, 0, SEEK_SET);
        // read value
        (void) read(watchFd, &value, 1);
    }
    return value;
}

void *IsrThread(void *args) {
    int ret;

    // check if thread has been initialized
    if ((watchFd < 0) || (pIsrFunction == NULL )) {
        fprintf(stderr, "Error: thread has not been initialized!\n");
        pthread_exit((int *) -1);
    }

    pthread_mutex_lock(pSctMutex);

    // wait until interrupt
    ret = WaitIsr();

    pthread_mutex_unlock(pSctMutex);

    // call callback function
    pIsrFunction(ret);

    // clean opened descriptors
    CleanIsrThread();

    return NULL;
}

void CleanIsrThread(void) {
    // close watched file
    if (watchFd) {
        close(watchFd);
        watchFd = -1;
    }

    // clean pointer to callback function
    if (pIsrFunction != NULL)
        pIsrFunction = NULL;
}
