#include "common.h"
#include <string.h>
#include "pthread.h"
#include "gpio.h"

const static char* intEdgeds[] = {
    "none",
    "rising",
    "falling",
    "both"
};

static int GpIOInit(unsigned char pin, unsigned char in, unsigned char edge);
static int GpIOUnInit(unsigned char pin);
static int GpIOWrite(unsigned char pin, const char *value);
static int GpIORead(unsigned char pin);

static int GpIOInit(unsigned char pin, unsigned char in, unsigned char edge) {
    char fName[128] = {};
    FILE *fd;

    sprintf(fName, "%s/gpio%d", GPIO_PATH, pin);

    // check if pin is not already exported
    if (access(fName, F_OK) < 0) {
        // export pin
        if ((fd = fopen(GPIO_PATH"/export", "w")) == NULL) {
            fprintf(stderr, "Error: Can not open GPIO export interface %s!\n", strerror(errno));
            return -1;
        }
        fprintf(fd, "%d\n", pin);
        fclose(fd);
    } else {
        // already exported
        return 1;
    }

    // set direction
    sprintf(fName, "%s/gpio%d/direction", GPIO_PATH, pin);
    if ((fd = fopen(fName, "w")) == NULL) {
        fprintf(stderr, "Error: Can not open GPIO direction interface %s!\n", strerror(errno));
        return -2;
    }

    fprintf(fd, "%s\n", in ? "in" : "out") ;
    fclose(fd) ;

    // check if edge specified
    if (in) {
        sprintf(fName, "%sgpio%d/edge", GPIO_PATH, pin) ;
        if ((fd = fopen(fName, "w")) == NULL) {
            fprintf(stderr, "Error: Can not open GPIO edge interface %s!\n", strerror(errno));
            return -3;
        }

        fprintf(fd, "%s\n", intEdgeds[edge]);
        fclose(fd);
    }
    return 0;
}

static int GpIOUnInit(unsigned char pin) {
    return 0;
}

static int GpIOWrite(unsigned char pin, const char *value) {
    char fName[128] = {};
    FILE *fd;

    sprintf(fName, "%sgpio%d/value", GPIO_PATH, pin);
    if ((fd = fopen(fName, "w")) == NULL) {
        fprintf(stderr, "Error: Can not open GPIO value interface %s!\n", strerror(errno));
        return -1;
    }

    fprintf(fd, "%s\n", value);
    fclose(fd);

    return 0;
}

static int GpIORead(unsigned char pin) {
    char fName[128] = {};
    FILE *fd;
    int value;

    sprintf(fName, "%sgpio%d/value", GPIO_PATH, pin);
    if ((fd = fopen(fName, "r")) == NULL) {
        fprintf(stderr, "Error: Can not open GPIO value interface %s!\n", strerror(errno));
        return -1;
    }

    fscanf(fd, "%d", &value);
    fclose(fd);

    return value;
}

void GpIOOutput(unsigned char pin, unsigned char value) {
    // check pin ranges
    if (pin < GPIO_PIN_MIN || pin > GPIO_PIN_MAX) {
        fprintf(stderr, "Error: entered pin %d is out of range\n!", pin);
        return;
    }

    // init GPIO
    if (GpIOInit(pin, 0, GPIO_INT_EDGE_NONE) < 0) {
        fprintf(stderr, "Error: Can not initialize GPIO pin %d!\n", pin);
        return;
    }

    // set GPIO value
    if (GpIOWrite(pin, value ? "1" : "0")) {
        fprintf(stderr, "Error: Can not set value %d on GPIO pin %d!\n", value, pin);
    }
}

int GpIOInputISR(unsigned char pin, unsigned char edge, void (*pFunction)(void)) {
    // check pin ranges
    if (pin < GPIO_PIN_MIN || pin > GPIO_PIN_MAX) {
        fprintf(stderr, "Error: entered pin %d is out of range\n!", pin);
        return -1;
    }

    // check edge ranges
    if ((edge < GPIO_INT_EDGE_NONE) || (edge >= GPIO_INT_EDGE_COUNT)) {
        fprintf(stderr, "Error: entered edge %d is out of range\n!", edge);
        return -2;
    }

    // init GPIO
    if (GpIOInit(pin, 1, edge) < 0) {
        fprintf(stderr, "Error: Can not initialize GPIO pin %d!\n", pin);
        return -3;
    }

    // init pthread to call interrupt handler
    if (pFunction != NULL) {
        // init threads
        if (InitIsrThread()) {
            fprintf(stderr, "Error: Can not initialize GPIO interrupt thread!\n");
            return -4;
        }
        // TODO: init thread
    }
}

int GpIOInput(unsigned char pin) {
    return GpIOInputISR(pin, GPIO_INT_EDGE_NONE, NULL);
}
