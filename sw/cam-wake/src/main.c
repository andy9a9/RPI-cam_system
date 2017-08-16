#include "common.h"
#include "gpio.h"
#include <sys/resource.h>

#define GPIO_PIN 17

int Help(void) {
    printf("This is help\n");
    return 1;
}

int main(int argc, char *argv[]) {
    int ret = 0;
    int i;
    char cmd[128] = CAM_SYSTEM_APP;

    printf("Starting cam-wake!\n");

    // check args
    if (argc > 1)
        // save binary location
        strcpy(cmd, argv[1]);

    // check if application is executable
    if (access(cmd, F_OK|X_OK) < 0) {
        fprintf(stderr, "Error: can not find cam-system application!\n");
        return Help();
    }

    // append configuration file location
    if (argc < 3) {
        strcat(cmd, CAM_SYSTEM_CFG " -i");
    } else {
        for (i = 2; i < argc; i++) {
            strcat(cmd, " ");
            strcat(cmd, argv[i]);
        }
    }

    // set priority to low
    setpriority(PRIO_PROCESS, getpid(), 19);

    // wait for interrupt
    ret = GpIOInputIsr(GPIO_PIN, GPIO_INT_EDGE_FALLING, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error: GPIO interrupt %i!\n", ret);
        return 2;
    }

    // deinitialize gpio
    if (GpIOUninit(GPIO_PIN) < 0) {
        fprintf(stderr, "Error: can not de-init GPIO!\n");
        return 3;
    }

    // wake-up cam-system application
    printf("Waking-up cam-system!\n");
    // run command
    ret = system(cmd);

    printf("Stopping cam-wake!\n");

    return ret;
}
