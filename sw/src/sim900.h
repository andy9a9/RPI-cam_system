#ifndef SIM900_H_
#define SIM900_H_

#include "serial.h"
#include <string>

// pins definitions //TODO: modify for GPIO
#define GPIO_SIM900_ON      2
#define GPIO_SIM900_REST    3

// status bits definition
#define MSK_STATUS_NONE         0
#define MSK_STATUS_INITIALIZED  1
#define MSK_STATUS_REGISTERED   2

// length for internal communication buffer
#define COMM_BUFF_SIZE 200

// common used strings
#define STR_AT "AT"
#define STR_ERR "ERROR"
#define STR_OK "OK"
#define STR_CR '\r'
#define STR_LF '\n'
#define STR_CRLF "\r\n"

// initial parameters settings
#define INIT_PARAM_SET_0 0
#define INIT_PARAM_SET_1 1

enum ComLineStatus {
    CLS_FREE = 0,   // line is free
    CLS_ATCMD,      // line is used by AT command + timeouts for response
    CLS_DATA,       // line is used for data
    CLS_COUNT
};

enum GSMStatus {
    GSM_ST_IDLE = 0,
    GSM_ST_READY,
    GSM_ST_ATTACHED,
    GSM_ST_ERROR,
    GSM_ST_TCP_SERVER_WAIT,
    GSM_ST_TCP_SERVER_CONNECTED,
    GSM_ST_TCP_CLIENT_CONNECTED,
    GSM_ST_COUNT
};

enum ATResponse {
    AT_RESP_NO_RESP = -1,   // nothing received
    AT_RESP_DIFF_RESP,      // response string is different from the response
    AT_RESP_OK,             // response string was included in the response
};

enum RXStatus {
    RX_ST_NOT_FINISHED = 0, // RX has not been finished yet
    RX_ST_FINISHED,         // finished, received some char
    RX_ST_FINISHED_STR_OK,  // finished, received expected string
    RX_ST_FINISHED_STR_ERR, // finished, received non-expected string
    RX_ST_TIMEOUT_ERR,      // finished, timeout elapsed
    RX_ST_LAST_ITEM         // init communication timeout
};

class CGSM {
public:
    CGSM();
    virtual ~CGSM();

    virtual bool Init(const BaudRate baudrate, const char *device);
    int SendATCmd(const char *ATcmd, unsigned long tmt, unsigned long maxCharsTmt,
        const char *expStr = NULL, unsigned char repeatCount = 0);
    int WaitResp(unsigned long tmt, unsigned long maxCharsTmt, const char *expStr = NULL);

    inline void SetCommStatus(unsigned char status) { m_commStatus = status; }
    inline int GetCommStatus() { return m_commStatus; }

    inline void SetGSMStatus(unsigned char status) { m_GSMStatus = status; }
    inline int GetGSMStatus() { return m_GSMStatus; }

    inline std::string GetCommBuff() { return std::string(m_commBuff); }

protected:
    void Echo(bool on);
    void InitParams(unsigned char params);

    inline void Write(char c) { return _Write(&c, 1); }
    inline void Write(char *data, size_t len = 0) { return _Write(data, len); }
    inline void Write(const char *data, size_t len = 0) { return _Write(data, len); }

    inline void WriteLn(char c) { return _WriteLn(&c, 1); }
    inline void WriteLn(char *data, size_t len = 0) { return _WriteLn(data, len); }
    inline void WriteLn(const char *data, size_t len = 0) { return _WriteLn(data, len); }

    size_t Read(char *pOut, size_t len = 0);

private:
    void _Write(const char *data, size_t len);
    void _WriteLn(const char *data, size_t len);
    bool CheckRcvdResp(const char *str);

private:
    CSerial *m_pSerial;
    char m_commBuff[COMM_BUFF_SIZE + 1];
    unsigned char m_commStatus;
    unsigned char m_GSMStatus;
};

class CSIM900: public CGSM {
public:
    bool Init(const BaudRate baudrate, const char *device);
    bool ForceON();

    std::string GetIMEI();
    std::string GetCCI();
    std::string GetIP();

    bool CheckRegistration();
    inline bool IsRegistered() { return m_registered; }

private:
    bool m_registered;
    bool m_initialized;
    friend class CCtrlGSM;
};

class CCtrlGSM {
public:
    CCtrlGSM();
    ~CCtrlGSM();

    bool Init(const BaudRate baudrate = BR_9600, const char *device = "ttyAMA0");
    bool AttachGPRS(const char *apn, const char *user, const char *pwd);
    bool DetachGPRS();

    bool HttpGET(const char *server, unsigned int port, const char *path, char *out = NULL, size_t outLen = 0);

private:
    bool ConnectTCP(const char *server, unsigned int port);
    bool DisconnectTCP();

private:
    CSIM900 *m_pSIM900;
    bool m_initialized;
    bool m_connected;
};

#endif // SIM900_H_
