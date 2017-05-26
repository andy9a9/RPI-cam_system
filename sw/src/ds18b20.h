#ifndef DS18B20_H_
#define DS18B20_H_

#include <string>
#include <vector>

#define OW_PATH "/sys/bus/w1/devices/"
#define OW_SLAVE "/w1_slave"
#define DS18_PREFIX "28-"

#define UNIT_CELSIUS 0
#define UNIT_FARENHEIT 1

class CDS18B20 {
public:
    CDS18B20();
    ~CDS18B20();

    bool Init();
    inline unsigned char GetDevCount() { return m_devCount; }
    inline unsigned char GetCurrIdx() { return m_currIdx; }

    float GetTemp(unsigned char idx = 0);

private:
    const char *m_addr;
    unsigned char m_unit;
    unsigned char m_devCount;
    unsigned char m_currIdx;
    std::vector<std::string> m_devices;
    std::string m_devPath;
};

#endif // DS18B20_H_
