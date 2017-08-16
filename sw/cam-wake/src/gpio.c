#include "common.h"
#include "interrupts.h"
#include "gpio.h"

const static char* intEdgeds[] = {
    "none",
    "rising",
    "falling",
    "both"
};

static int GpIOInit(unsigned char pin, unsigned char in, unsigned char edge);
static int GpIORead(unsigned char pin);
static int GpIOWrite(const char *pGpioFile, const char *value, unsigned int wait);

static int GpIOInit(unsigned char pin, unsigned char in, unsigned char edge) {
    char fName[64] = {};

    sprintf(fName, "%s/gpio%d", GPIO_PATH, pin);

    // check if pin is not already exported
    if (access(fName, F_OK) < 0) {
        sprintf(fName, "%d", pin);
        // export pin
        if (GpIOWrite(GPIO_PATH"/export", fName, 0) < 0) {
            fprintf(stderr, "Error: can not open GPIO export interface!\n");
            return -1;
        }
    } else {
        // already exported
        return 1;
    }

    // set direction
    sprintf(fName, "%s/gpio%d/direction", GPIO_PATH, pin);
    if (GpIOWrite(fName, in ? "in" : "out", 1000) < 0) {
        fprintf(stderr, "Error: can not open GPIO direction interface!\n");
        return -2;
    }

    // check if edge specified
    if (edge) {
        sprintf(fName, "%s/gpio%d/edge", GPIO_PATH, pin);
        if (GpIOWrite(fName, intEdgeds[edge], 1000) < 0) {
            fprintf(stderr, "Error: can not open GPIO edge interface!\n");
            return -3;
        }
    }
    return 0;
}

int GpIOUninit(unsigned char pin) {
    char tmpBuff[4] = {};

    // unexport pin
    sprintf(tmpBuff, "%d", pin);
    if (GpIOWrite(GPIO_PATH"/unexport", tmpBuff, 0) < 0) {
        fprintf(stderr, "Error: can not open GPIO unexport interface!\n");
        return -1;
    }

    return 0;
}

static int GpIORead(unsigned char pin) {
    char fName[64] = {};
    FILE *fd;
    int value;

    sprintf(fName, "%s/gpio%d/value", GPIO_PATH, pin);
    if ((fd = fopen(fName, "r")) == NULL) {
        fprintf(stderr, "Error: can not open GPIO value interface: %s!\n", strerror(errno));
        return -1;
    }

    fscanf(fd, "%d", &value);
    fclose(fd);

    return value;
}

static int GpIOWrite(const char *pGpioFile, const char *value, unsigned int wait) {
    int i;
    int fd;
    int ret = 0;

    // check file and data
    if (pGpioFile == NULL || value == NULL) {
        fprintf(stderr, "Error: file or value are NULL!\n");
        return -1;
    }

    // make some delay before file creation
    for (i = 0;; i += 20) {
        // open file for writing
        if ((fd = open(pGpioFile, O_WRONLY, 0)) >= 0) break;
        // check wait condition
        if (i >= wait) break;
        // make some wait
        usleep(20 * 1000);
    }

    // check if file has been successfully open
    if (fd < 0) {
        fprintf(stderr, "Error: can not open file %s, err:%s!", pGpioFile, strerror(errno));
        return -2;
    }

    // get data length
    int len = strlen(value);

    // write data to file
    if ((ret = write(fd, value, len)) < 0) {
        fprintf(stderr, "Error: can not write value to file %s, err:%s!", pGpioFile, strerror(errno));
        ret = -3;
    }

    // close file
    close(fd);

    return (ret == len);
}

void GpIOOutput(unsigned char pin, unsigned char value) {
    char fName[64] = {};

    // check pin ranges
    if (pin < GPIO_PIN_MIN || pin > GPIO_PIN_MAX) {
        fprintf(stderr, "Error: entered pin %d is out of range!\n", pin);
        return;
    }

    // init GPIO
    if (GpIOInit(pin, 0, GPIO_INT_EDGE_NONE) < 0) {
        fprintf(stderr, "Error: can not initialize GPIO pin %d!\n", pin);
        return;
    }

    // set GPIO value
    sprintf(fName, "%s/gpio%d/value", GPIO_PATH, pin);
    if (GpIOWrite(fName, value ? "1" : "0", 0) < 0) {
        fprintf(stderr, "Error: can not set value %d on GPIO pin %d!\n", value, pin);
        return;
    }
}

int GpIOInputIsr(unsigned char pin, unsigned char edge, void (*pFunction)(int)) {
    // check pin ranges
    if (pin < GPIO_PIN_MIN || pin > GPIO_PIN_MAX) {
        fprintf(stderr, "Error: entered pin %d is out of range!\n", pin);
        return -1;
    }

    // check edge ranges
    if ((edge < GPIO_INT_EDGE_NONE) || (edge >= GPIO_INT_EDGE_COUNT)) {
        fprintf(stderr, "Error: entered edge %d is out of range!\n", edge);
        return -2;
    }

    // init GPIO
    if (GpIOInit(pin, 1, edge) < 0) {
        fprintf(stderr, "Error: can not initialize GPIO pin %d!\n", pin);
        return -3;
    }

    // get actual value
    int value = GpIORead(pin);
    if (value < 0) {
        fprintf(stderr, "Error: can not read value from GPIO pin %d!\n", pin);
        return -4;
    }

    // path of watching file
    char fName[64] = {};
    sprintf(fName, "%s/gpio%d/value", GPIO_PATH, pin);

    // init threads
    if (InitIsr(fName, pFunction)) {
        fprintf(stderr, "Error: can not initialize GPIO interrupt thread!\n");
        return -5;
    }

    // init pthread to call interrupt handler
    if (pFunction == NULL) {
        // wait until interrupt
        value = WaitIsr();
    } else {
        // isr thread
        pthread_t thread;

        // create isr thread
        if (pthread_create(&thread, NULL, IsrThread, NULL)) {
            fprintf(stderr, "Error: can not create interrupt thread!\n");
            return -6;
        }
    }
    return value;
}

int GpIOInput(unsigned char pin) {
    return GpIOInputIsr(pin, GPIO_INT_EDGE_NONE, NULL);
}
