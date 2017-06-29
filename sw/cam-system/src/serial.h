#ifndef SERIAL_H_
#define SERIAL_H_

#include <termios.h>
#include <string>

typedef enum BaudRate {
    BR_0 = 0,
    BR_1200 = B1200,
    BR_1800,
    BR_2400,
    BR_4800,
    BR_9600,
    BR_19200,
    BR_38400,
    BR_115200 = B115200
} BaudRate;

// message timeout * 0.1[s]
#define SERIAL_TIMEOUT 100
// device node
#define DEV_PATH "/dev/"
// max size for rx/tx buffer
#define MAX_BUFF_SIZE 1024
// default serial node
#define DEF_SERIAL_NODE "ttyAMA0"

class CSerial {
public:
    CSerial();
    ~CSerial();

    bool Open(const BaudRate baudrate, const char *device = DEF_SERIAL_NODE);
    void Close();
    inline bool IsConnected() { return m_isOpened; }
    inline unsigned int GetBaudRate() { return m_baudRate; }
    inline std::string GetDeviceName() { return m_devNode; }

    void Putc(char c);
    void Puts(const char *data, size_t len);

    void Printf(const char *message, ...);
    void Printf(const std::string &message);

    char Getc();

    size_t DataAvailable();
    void Flush();

private:
    enum BaudRate m_baudRate;
    int m_devFd;
    std::string m_devNode;
    bool m_isOpened;
};

#endif // SERIAL_H_
