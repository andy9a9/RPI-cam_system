#include "common.h"
#include "logger.h"
#include <stdarg.h>
#include <sys/ioctl.h>

#include "serial.h"

CSerial::CSerial() :
    m_baudRate(BR_0),
    m_devFd(-1),
    m_isOpened(false) {
}

CSerial::~CSerial() {
    Close();
}

bool CSerial::Open(const BaudRate baudrate, const char *device) {
    if (m_isOpened) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "device %s is already opened!", m_devNode.c_str());
        return false;
    }

    // check if device is entered
    if (device == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "device %s is null!", device);
        return false;
    }

    // set device absolute path
    m_devNode = DEV_PATH + std::string(device, strlen(device));

    // open device
    // rd+rw, without terminal access, non-blocking mode
    if ((m_devFd = open(m_devNode.c_str(), O_RDWR | O_NOCTTY | O_NDELAY | O_NONBLOCK)) < 0) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "can not access to device %s!", m_devNode.c_str());
        m_devNode.clear();
        return false;
    }

    struct termios options = {};
    // get actual options
    tcgetattr(m_devFd, &options);

    // set raw options
    cfmakeraw(&options);
    // set in/out baudrate
    cfsetispeed(&options, baudrate);
    cfsetospeed(&options, baudrate);

    options.c_cflag |= (CLOCAL | CREAD); // ignore modem status lines, enable receiver
    options.c_cflag &= ~PARENB; // parity enable
    options.c_cflag &= ~CSTOPB; // enable stop bit
    options.c_cflag &= ~CSIZE; // set 8-bit size
    options.c_cflag |= CS8;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;

    options.c_cc[VMIN] = 0;
    options.c_cc[VTIME] = SERIAL_TIMEOUT;

    try {
        // set parameters to device
        tcflush(m_devFd, TCIFLUSH);
        tcsetattr(m_devFd, TCSANOW, &options);
    } catch (std::exception &ex) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "can not set parameters to device %s!", m_devNode.c_str());

        // cleaning
        m_devNode.clear();
        close(m_devFd);
        m_devFd = -1;
        return false;
    }
    // set connection flag
    m_isOpened = true;

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "device %s@%i was successfully initialized", m_devNode.c_str(), baudrate);

    return true;
}

void CSerial::Close() {
    if (!m_isOpened) return;

    if (m_devFd) {
        // close device descriptor
        close(m_devFd);
        m_devFd = -1;
    }

    m_isOpened = false;
}

void CSerial::Putc(char c) {
    // check connection
    if ((!m_isOpened) || (!m_devFd)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "device %s is not open!", m_devNode.c_str());
        return;
    }

    // write data to device
    if (write(m_devFd, &c, 1) != 1)
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "can not write data to device %s!", m_devNode.c_str());
}

void CSerial::Puts(const char *data, size_t len) {
    // check connection
    if ((!m_isOpened) || (!m_devFd)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "device %s is not open!", m_devNode.c_str());
        return;
    }

    // check output data
    if (data == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "output data are null!");
        return;
    }

    // write data to device
    if (write(m_devFd, data, len) != (ssize_t) len)
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "can not write data to device %s!", m_devNode.c_str());
}

void CSerial::Printf(const char *message, ...) {
    int size;
    char *msg = NULL;

    va_list args;
    // format string with message
    va_start(args, message);

    // get formated string length
    size = vsnprintf(NULL, 0, message, args) + 1;
    // check max size
    if (size > (MAX_BUFF_SIZE - 1)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "tx message is too long!");
        return;
    }

    // allocate new size
    msg = new char[size];

    // format message
    size = vsnprintf(msg, size, message, args);
    va_end(args);

    // send the data
    Puts(msg, size);

    // clean new buffer
    free(msg);
    msg = NULL;
}

void CSerial::Printf(const std::string &message) {
    // TODO: make std::string access
}

char CSerial::Getc() {
    // check connection
    if ((!m_isOpened) || (!m_devFd)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "device %s is not open!", m_devNode.c_str());
        return 0;
    }

    char c;

    // read data from device
    if (read(m_devFd, &c, 1) != 1) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "can not read data from deice() %s", m_devNode.c_str());
        c = -1;
    }

    return c;
}

size_t CSerial::DataAvailable() {
    // check connection
    if ((!m_isOpened) || (!m_devFd)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "device %s is not open!", m_devNode.c_str());
        return 0;
    }

    size_t count = 0;

    try {
        // get count of available data to read
        if (ioctl(m_devFd, FIONREAD, &count) < 0) {
            //TODO: timeout problem?
            //CLogger::GetLogger()->LogPrintf(LL_ERROR, "cant DataAvailable() %s", ex.what());
        }
    } catch (std::exception &ex) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "DataAvailable() %s", ex.what());
        count = 0;
    }

    return count;
}

void CSerial::Flush() {
    // check connection
    if ((!m_isOpened) || (!m_devFd)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "device %s is not open!", m_devNode.c_str());
        return;
    }

    try {
        // flush buffer
        tcflush(m_devFd, TCIOFLUSH);
    } catch (std::exception &ex) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "Flush() %s", ex.what());
    }
}
