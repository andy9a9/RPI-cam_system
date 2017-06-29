#ifndef GPIO_H_
#define GPIO_H_

#include <string>

// min max GPIO pins
#define GPIO_PIN_MIN 2
#define GPIO_PIN_MAX 24

#define AS_ROOT "sudo echo "
#define GPIO_PATH "/sys/class/gpio/"

class CGpIOBase {
public:
    CGpIOBase();
    ~CGpIOBase();

protected:
    static bool RunCmd(const std::string &cmd);
    static bool WriteFile(const std::string &file, const char *data, int wait = 0);
    static std::string ReadFile(const std::string &file);
};

class CGpIOAny: public CGpIOBase {
public:
    explicit CGpIOAny(unsigned int pin);
    ~CGpIOAny();

    inline unsigned int GetPin() const { return m_pin; }

protected:
    void SetDir(const bool in);
    void SetVal(const bool on);

private:
    const unsigned int m_pin;
};

class CGpIOInput: public CGpIOAny {
public:
    explicit CGpIOInput(unsigned int pin);
    ~CGpIOInput();

    bool GetValue();
};

class CGpIOOutput: public CGpIOAny {
public:
    explicit CGpIOOutput(unsigned int pin);
    ~CGpIOOutput();

    void SetValue(const bool on);

private:
    bool m_act;
};

#endif // GPIO_H_
