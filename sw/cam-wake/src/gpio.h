#ifndef GPIO_H
#define GPIO_H

// min max GPIO pins
#define GPIO_PIN_MIN 2
#define GPIO_PIN_MAX 24

#define AS_ROOT "sudo echo "
#define GPIO_PATH "/sys/class/gpio"

enum en_interrupt_edge {
    GPIO_INT_EDGE_NONE = 0,
    GPIO_INT_EDGE_RISING,
    GPIO_INT_EDGE_FALLING,
    GPIO_INT_EDGE_BOTH,
    GPIO_INT_EDGE_COUNT
};

int GpIOInputISR(unsigned char pin, unsigned char edge, void (*pFunction)(void));
void GpIOOutput(unsigned char pin, unsigned char value);
int GpIOInput(unsigned char pin);
int GpIOUninit(unsigned char pin);

#endif // GPIO_H
