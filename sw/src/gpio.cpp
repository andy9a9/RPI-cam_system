#include "common.h"
#include "logger.h"
#include <fstream>

#include "gpio.h"

CGpIOBase::CGpIOBase() {
}

CGpIOBase::~CGpIOBase() {
}

bool CGpIOBase::RunCmd(const std::string &cmd) {
    if (cmd.empty()) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "command to run is missing!");
        return false;
    }
    // execute the command
    if (system(cmd.c_str()) < 0) return false;
    return true;
}

bool CGpIOBase::WriteFile(const std::string &file, const char *data, int wait) {
    // check file and data
    if (file.empty() || data == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "file or data are NULL");
        return false;
    }

    int fd, ret = 0;

    // make some delay before file creation
    for (int i = 0;; i += 20) {
        // open file for writing
        if ((fd = open(file.c_str(), O_WRONLY, 0)) >= 0) break;
        // check wait condition
        if (i >= wait) break;
        // make some wait
        usleep(20 * 1000);
    }

    // check if file has been successfully open
    if (fd < 0) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "Unable to open file + for writing!", file.c_str());
        return false;
    }

    // get data length
    int len = strlen(data);

    // write data to file
    if ((ret = write(fd, data, len)) < 0) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "Unable write value to file %s!", file.c_str());
        ret = -1;
    }

    // close file
    close(fd);

    return (ret == len);
}

std::string CGpIOBase::ReadFile(const std::string &file) {
    // check file and data
    if (file.empty()) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "file for reading is empty");
        return std::string();
    }

    int fd;

    // open file for reading
    if ((fd = open(file.c_str(), O_RDONLY, 0)) < 0) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "Unable to open file %s for reading!", file.c_str());
        return std::string();
    }

    char buf[128];
    int cnt;

    // read the file
    if ((cnt = read(fd, buf, arraysize(buf) - 1)) < 0) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "Unable to read data from file %s!", file.c_str());
        // empty string
        buf[0] = '\0';
    } else buf[cnt] = '\0';

    // close file
    close(fd);

    return buf;
}

CGpIOAny::CGpIOAny(unsigned int pin) :
    m_pin(pin) {
    // check ranges
    if (pin < GPIO_PIN_MIN || pin > GPIO_PIN_MAX) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "GPIO pin %i is out of range!", pin);
        return;
    }

    // run command to export pin
    verify(RunCmd(AS_ROOT "\"" + ToString(m_pin) + "\" > "GPIO_PATH"export"));
}

CGpIOAny::~CGpIOAny() {
    // run command to unexport pin
    verify(RunCmd(AS_ROOT "\"" + ToString(m_pin) + "\" > "GPIO_PATH"unexport"));
}

void CGpIOAny::SetDir(const bool in) {
    // run command to set pin direction
    verify(WriteFile(GPIO_PATH"gpio" + ToString(m_pin) + "/direction", in ? "in" : "out", 1000));
}

CGpIOInput::CGpIOInput(unsigned int pin) :
    CGpIOAny(pin) {
    // set pin as input
    SetDir(true);
}

CGpIOInput::~CGpIOInput() {
}

bool CGpIOInput::GetValue() {
    // get pin value
    return !ReadFile(GPIO_PATH"gpio" + ToString(GetPin()) + "/value").compare("1");
}

CGpIOOutput::CGpIOOutput(unsigned int pin) :
    CGpIOAny(pin) {
    // set pin as output
    SetDir(false);
    m_act = true;

    // set pin default value to off
    SetValue(false);
}

CGpIOOutput::~CGpIOOutput() {
    SetValue(false);
}

void CGpIOOutput::SetValue(const bool on) {
    // check if value is going to be changed
    if (on == m_act) return;

    // set pin value
    verify(WriteFile(GPIO_PATH"gpio" + ToString(GetPin()) + "/value", on ? "1" : "0"));
}

/*CGPIO::CGPIO() {
 m_pin = 0;
 }

 CGPIO::CGPIO(unsigned char pin) {
 // check ranges
 if (pin < GPIO_PIN_MIN || pin > GPIO_PIN_MAX) {
 CLogger::GetLogger()->LogPrintf(LL_ERROR, "GPIO pin %i is out of range!", pin);
 return;
 }
 // set GPIO pin
 m_pin = pin;
 }

 CGPIO::~CGPIO() {
 }

 bool CGPIO::Export() {
 return WriteGPIO("export", &m_pin);
 }

 bool CGPIO::UnExport() {
 return WriteGPIO("unexport", &m_pin);
 }

 bool CGPIO::SetDir(const bool in) {
 return (WriteGPIO("gpio/" + ToString(m_pin) + "/direction", in ? "in" : "out"));
 }

 bool CGPIO::SetVal(const bool value) {
 return (WriteGPIO("gpio/" + ToString(m_pin) + "/value", value ? "1" : "0"));
 }

 bool CGPIO::GetVal(const bool &value) {
 std::string strVal;
 WriteGPIO("gpio/" + ToString(m_pin) + "/value", strVal);
 // convert string value to boolean
 return (strVal == "1" ? true : false);
 }

 bool CGPIO::WriteGPIO(const std::string path, const std::string value) {

 // open file for writing
 std::ofstream gpioFile(std::string(GPIO_PATH + path).c_str(), std::ofstream::out);

 bool ret = true;

 // check if gpio file is open
 if (!gpioFile.is_open()) {
 CLogger::GetLogger()->LogPrintf(LL_ERROR, "Unable write value to GPIO pin %i!", m_pin);
 ret = false;
 }

 // write value to pin
 gpioFile << value;
 // close file
 gpioFile.close();

 return ret;
 }

 bool CGPIO::ReadGPIO(const std::string path, std::string &value) {

 // open file for reading
 std::ifstream gpioFile(std::string(GPIO_PATH + path).c_str(), std::ifstream::in);

 bool ret = true;

 // check if gpio file is open
 if (!gpioFile.is_open()) {
 CLogger::GetLogger()->LogPrintf(LL_ERROR, "Unable read value from GPIO pin %i!", m_pin);
 ret = false;
 }

 // read data from pin
 gpioFile >> value;
 // close file
 gpioFile.close();

 return ret;
 }*/

