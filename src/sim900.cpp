#include "common.h"
#include "logger.h"
#include "gpio.h"
#include <time.h>

#include "sim900.h"

CGSM::CGSM() {
    m_pSerial = NULL;
    memset(m_commBuff, 0x00, sizeof(m_commBuff));
    m_commStatus = CLS_FREE;
    m_GSMStatus = GSM_ST_IDLE;
}

CGSM::~CGSM() {
    // clean serial
    if (m_pSerial != NULL) {
        m_pSerial->Close();
        delete m_pSerial;
    }
    m_pSerial = NULL;
    memset(m_commBuff, 0x00, sizeof(m_commBuff));
}

bool CGSM::Init(const BaudRate baudrate, const char *device) {
    // init serial
    m_pSerial = new CSerial();
    // set baudrate
    return m_pSerial->Open(baudrate, device);
}

int CGSM::SendATCmd(const char *ATcmd, unsigned long tmt, unsigned long maxCharsTmt,
    const char *expStr, unsigned char repeatCount) {
    int ret = AT_RESP_NO_RESP;
    int status;

    // check AT command
    if (ATcmd == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "AT command is missing!");
        return ret;
    }

    // loop over attempt to retry
    for (int i = 0; i < repeatCount; i++) {
        // wait 500ms before sending the next command
        if (i) usleep(500000);

        // send AT command to device
        m_pSerial->Printf("%s"STR_CRLF, ATcmd);

        // wait for response and get the status
        status = WaitResp(tmt, maxCharsTmt);
        // check status
        if (status == RX_ST_FINISHED) {
            // check if response is expected
            if (expStr != NULL) {
                if (!CheckRcvdResp(expStr)) {
                    ret = AT_RESP_NO_RESP;
                    continue;
                }
            }
            // response is ok
            ret = AT_RESP_OK;
            break;
        } else ret = AT_RESP_NO_RESP;
    }

    return ret;
}

void CGSM::_Write(const char *data, size_t len) {
    // check data to write
    if (data == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "data to write are missing!");
        return;
    }

    // write data to device
    m_pSerial->Puts(data, len);
}

void CGSM::_WriteLn(const char *data, size_t len) {
    // check data to write
    if (data == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "data to write are missing!");
        return;
    }

    // write data to device
    m_pSerial->Puts(data, len);
    m_pSerial->Putc(STR_CR); // \r
    m_pSerial->Putc(STR_LF); // \n
}

size_t CGSM::Read(char *pOut, size_t len) {
    // check if output buffer is not null
    if (pOut == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "output buffer is null!");
        return 0;
    }

    size_t idx = 0;
    char c;

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "waiting for incoming data from device");

    // wait while incoming data are not available
    while (!m_pSerial->DataAvailable()) usleep(100000);

    while (m_pSerial->DataAvailable() && (idx < len -1)) {
        // skip null data
        if ((c = m_pSerial->Getc() != 0)) {
            // save data to output buffer
            *(pOut + idx++) = c;
        }
        usleep(1000);
    }

    // terminate output buffer
    *(pOut + (len - 1)) = '\0';

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "incoming data: %s", pOut);

    return idx;
}

void CGSM::Echo(bool on) {
    m_commStatus = CLS_ATCMD;

    // write data
    Write("ATE");
    Write(on ? "1" : "0");
    Write(STR_CR);
    usleep(500000);

    m_commStatus = CLS_FREE;
}

void CGSM::InitParams(unsigned char params) {
    // TODO: is it necessary?
}

int CGSM::WaitResp(unsigned long tmt, unsigned long maxCharsTmt, const char *expStr) {
    unsigned int status = RX_ST_NOT_FINISHED;
    bool rxStarted = false;

    size_t recDataCount;
    unsigned int dataToProcess = 0;

    char *pCommBuff = m_commBuff;

    m_commBuff[0] = '\0';

    // clear rx buffer
    m_pSerial->Flush();

    // get start time
    unsigned long tsStart = GetTimeMSec();

    // wait for response
    do {
        // check if receiving started
        if (!rxStarted) {
            // check if data are available
            if (m_pSerial->DataAvailable()) {
                // reset timer
                tsStart = GetTimeMSec();
                // set flag that receiving started
                rxStarted = true;
            } else {
                // check timeout
                if ((unsigned long)(GetTimeMSec() - tsStart) >= tmt) {
                    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "Reception timeout occurred");
                    // clean receiving buffer
                    m_commBuff[0] = '\0';
                    status = RX_ST_TIMEOUT_ERR;
                }
            }
        }

        // check if receiving was started
        if (rxStarted) {
            // get available data size
            if ((recDataCount = m_pSerial->DataAvailable())) {
                // reset timer
                tsStart = GetTimeMSec();
            }

            // read all received bytes
            while (recDataCount--) {
                // save data with size checking
                if (dataToProcess++ < COMM_BUFF_SIZE) {
                    // get data from device
                    *pCommBuff++ = m_pSerial->Getc();
                    *pCommBuff = '\0';
                } else {
                    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "Received data %c is out of range", m_pSerial->Getc());
                }
            }

            // check interchar timeout
            if ((unsigned long)(GetTimeMSec() - tsStart) >= maxCharsTmt) {
                CLogger::GetLogger()->LogPrintf(LL_DEBUG, "Receiving was finished");
                // terminate buffer
                *pCommBuff++ = '\0';
                status = RX_ST_FINISHED;
            }
        }
    } while(status == RX_ST_NOT_FINISHED);


    // clean pointer
    pCommBuff = NULL;

    // check if string verifying is expected
    if (expStr != NULL) {
        if (status == RX_ST_FINISHED) {
            // check received string
            if (CheckRcvdResp(expStr)) status = RX_ST_FINISHED_STR_OK;
            else status = RX_ST_FINISHED_STR_ERR;
        }
    }

    return status;
}

bool CGSM::CheckRcvdResp(const char *str) {
    if (str == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "String for comparison is missing!");
        return false;
    }

    // compare strings
    if (strstr(m_commBuff, str) == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "Received answer %s is different from expectation %s",
            m_commBuff, str);
        return false;
    }

    return true;
}

bool CSIM900::Init(const BaudRate baudrate, const char *device) {
    int retry;
    bool turnedON = false;
    bool noResp = false;
    m_initialized = false;

    // set GPIO pins modes
    CGpIOOutput gpioON(GPIO_SIM900_ON);
    CGpIOOutput gpioRESET(GPIO_SIM900_REST);

    // set statuses
    SetCommStatus(CLS_ATCMD);
    SetGSMStatus(GSM_ST_IDLE);

    // init gsm class with serial
    if (!CGSM::Init(baudrate, device)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "Error in GSM class initialization!");
        return m_initialized;
    }

    // if no-reply we turn to turn on the module
    for (retry = 0; retry < 3; retry++) {
        if ((SendATCmd(STR_AT, 500, 100, STR_OK STR_CRLF, 5) == AT_RESP_NO_RESP) && (!turnedON)) {
            CLogger::GetLogger()->LogPrintf(LL_DEBUG, "SIM900: No response");

            // turn on module
            gpioON.SetValue(true);
            sleep(2);
            gpioON.SetValue(false);
            sleep(10);
            noResp = true;
            // wait for response
            WaitResp(1000, 1000);
        } else {
            CLogger::GetLogger()->LogPrintf(LL_DEBUG, "SIM900: next step to init device");
            noResp = false;
            // wait for response
            WaitResp(1000, 1000);
        }
    }

    // check if device is started
    if (SendATCmd(STR_AT, 500, 100, STR_OK STR_CRLF, 5)) {
        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "SIM900: started");
        turnedON = true;
    }

    // check started status
    if (retry == 3 && noResp) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "SIM900: doesn't answer");

        gpioON.SetValue(true);
        sleep(2);
        gpioON.SetValue(false);
        sleep(10);

        return m_initialized;
    }

    // set comm status
    SetCommStatus(CLS_FREE);

    // set default settings
    if (turnedON) {
        WaitResp(50, 50);
        InitParams(INIT_PARAM_SET_0);
        InitParams(INIT_PARAM_SET_1);
        Echo(false);

        // set status to ready
        SetGSMStatus(GSM_ST_READY);
    } else {
        // TODO:try to fix problems with 115200 baudrate
        return m_initialized;
    }

    m_initialized = true;
	return m_initialized;
}

std::string CSIM900::GetIMEI() {
    int status;
    std::string ret = std::string();

    // write AT command to get IMEI
    WriteLn("AT+GSN");

    // get IMEI
    if ((status = WaitResp(5000, 50, STR_OK)) != RX_ST_FINISHED_STR_ERR) {
        // get data from device
        ret = GetCommBuff();
        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "SIM900: IMEI:%s", ret.c_str());
    }

    return ret;
}

std::string CSIM900::GetCCI() {

    // check status
    if (GetCommStatus() != GSM_ST_READY) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "SIM900: GSM status needs to be ready!");
        return std::string();
    }

    // write AT command to get CCI
    WriteLn("AT+QCCID");

    // TODO: read data from device
}

CCtrlGSM::CCtrlGSM() {
    m_pSIM900 = NULL;
    m_initialized = false;
    m_connected = false;
}

CCtrlGSM::~CCtrlGSM() {
    if (m_pSIM900 != NULL) {
        DisconnectTCP();
        delete m_pSIM900;
    }
    m_pSIM900 = NULL;
}

bool CCtrlGSM::Init(const BaudRate baudrate, const char *device) {
    m_initialized = false;

    // initialize GSM + SIM900 modules
    m_pSIM900 = new CSIM900();
    if (m_pSIM900->Init(baudrate, device)) m_initialized = true;

    return m_initialized;
}

bool CCtrlGSM::AttachGPRS(const char *apn, const char *user, const char *pwd) {
    if (!m_initialized) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "GSM module is not initialized!");
        return false;
    }

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "CCtrlGSM: establishing connection");

    // some delay
    sleep(5);

    // wait for response
    m_pSIM900->WaitResp(50, 50);
    // get local IP address
    m_pSIM900->WriteLn("AT+CIFSR");
    // wait and check answer
    if (m_pSIM900->WaitResp(5000, 50, STR_ERR) != RX_ST_FINISHED_STR_ERR) {
        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "CCtrlGSM: Already have an IP");

        // close connection
        m_pSIM900->WriteLn("AT+CIPCLOSE");
        m_pSIM900->WaitResp(50, 50);
        sleep(2);

        // de-configure module as server
        m_pSIM900->WriteLn("AT+CIPSERVER=0");
        m_pSIM900->WaitResp(5000, 50, "ERROR");

        m_connected = false;
    } else {
        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "CCtrlGSM: creating new connection");

        // close previous connection
        m_pSIM900->WriteLn("AT+CIPSHUT");

        // check response
        if (m_pSIM900->WaitResp(500, 50, "SHUT OK") == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "CCtrlGSM: can not close connection!");
            m_connected = false;
            return m_connected;
        }

        // make some delay
        sleep(1);

        // write data for connection to APN
        m_pSIM900->Write("AT+CSTT=\"");
        m_pSIM900->Write(apn);
        m_pSIM900->Write("\",\"");
        m_pSIM900->Write(user);
        m_pSIM900->Write("\",\"");
        m_pSIM900->Write(pwd);
        m_pSIM900->Write("\""+STR_CR);
        // check response
        if (m_pSIM900->WaitResp(500, 50, STR_OK) == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "CCtrlGSM: can not connect to APN!");
            m_connected = false;
            return m_connected;
        }

        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "CCtrlGSM: connected to APN");
        sleep(5);

        // create connection to with GPRS
        m_pSIM900->Write("AT+CIICR");
        // check response
        if (m_pSIM900->WaitResp(10000, 50, STR_OK) == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "CCtrlGSM: can not create connection with GPRS!");
            m_connected = false;
            return m_connected;
        }

        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "CCtrlGSM: connected with GRPS");

        // get local IP address
        m_pSIM900->WriteLn("AT+CIFSR");
        // check response
        if (m_pSIM900->WaitResp(5000, 50, STR_ERR) != RX_ST_FINISHED_STR_OK) {
            CLogger::GetLogger()->LogPrintf(LL_DEBUG, "CCtrlGSM: IP address was assigned");

            // set gsm status as attached
            m_pSIM900->SetGSMStatus(GSM_ST_ATTACHED);

            m_connected = true;
            return m_connected;
        }

        m_connected = false;
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "CCtrlGSM: no IP address after connection!");

    }

    return m_connected;
}

bool CCtrlGSM::DetachGPRS() {
    if (!m_initialized) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "CCtrlGSM: GSM module is not initialized!");
        return false;
    }

    // check status
    if (m_pSIM900->GetGSMStatus() == GSM_ST_IDLE) return true;

    // disconnect from GPRS (0-detach)
    m_pSIM900->WriteLn("AT+CGATT=0");
    // check response
    if (m_pSIM900->WaitResp(5000, 50, STR_OK) != RX_ST_FINISHED_STR_ERR) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "CCtrlGSM: can not close connection!");

        // set error status
        m_pSIM900->SetGSMStatus(GSM_ST_ERROR);
        return false;
    }

    // set status as disconnect
    m_pSIM900->SetGSMStatus(GSM_ST_READY);
    m_connected = false;
    return true;
}

bool CCtrlGSM::ConnectTCP(const char *server, unsigned int port) {
    // check if is connected
    if (!m_connected) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "CCtrlGSM: no GPRS connection!");
        return false;
    }

    // start tcp connection
    m_pSIM900->Write("AT+CIPSTART=\"TCP\",\"");
    m_pSIM900->Write(server);
    m_pSIM900->Write("\",");
    m_pSIM900->WriteLn(ToASCII(port));
    // check response
    if (m_pSIM900->WaitResp(1000, 200, STR_OK) == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "CCtrlGSM: can not start TCP connection!");
        return false;
    }

    // get last response
    const std::string str = m_pSIM900->GetCommBuff();

    // check connection status
    if (!str.compare("CONNECT OK")) {
        if (m_pSIM900->WaitResp(15000, 200, STR_OK) == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "CCtrlGSM: can not connect to server!");
            CLogger::GetLogger()->LogPrintf(LL_DEBUG, "CCtrlGSM: server response: %s", str.c_str());
            return false;
        }
    }

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "CCtrlGSM: connected to server: %s", server);
    sleep(3);

    // open connection for data sending
    m_pSIM900->WriteLn("AT+CIPSEND");
    if (m_pSIM900->WaitResp(5000, 200, ">") == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "CCtrlGSM: can not open connection for data sending!");
        return false;
    }

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "CCtrlGSM: opened connection for data sending");
    sleep(4);

    return true;
}

bool CCtrlGSM::DisconnectTCP() {
    // check if is connected
    if (!m_connected) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "CCtrlGSM: no GPRS connection!");
        return false;
    }

    // disconnect from server
    m_pSIM900->WriteLn("AT+CIPCLOSE");

    // status handling
    if (m_pSIM900->GetGSMStatus() == GSM_ST_TCP_CLIENT_CONNECTED)
        m_pSIM900->SetGSMStatus(GSM_ST_ATTACHED);
    else m_pSIM900->SetGSMStatus(GSM_ST_TCP_SERVER_WAIT);

    return true;
}

bool CCtrlGSM::HttpGET(const char *server, unsigned int port, const char *path, char *out, size_t outLen) {
    bool connected = false;

    // check server and output
    if (server == NULL || path == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "CCtrlGSM: server or path for connection are missing!");
        return false;
    }

    for (int retry = 0; retry < 3; retry++) {
        // try to connect to server
        if (ConnectTCP(server, port)) {
            CLogger::GetLogger()->LogPrintf(LL_INFO, "CCtrlGSM: connected to server %s", server);
            connected = true;
            break;
        }
        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "CCtrlGSM: connecting....%1", retry);
    }

    // check connection status
    if (!connected) {
        CLogger::GetLogger()->LogPrintf(LL_INFO, "CCtrlGSM: not connected to server %s", server);
        return false;
    }

    // write data to server
    m_pSIM900->Write("GET ");
    m_pSIM900->Write(path);
    m_pSIM900->Write(" HTTP/1.0\r\nHost: ");
    m_pSIM900->Write(server);
    m_pSIM900->Write(STR_CRLF);
    m_pSIM900->Write("User-Agent: Rpi-DEV");
    m_pSIM900->Write(STR_CRLF STR_CRLF);
    m_pSIM900->Write(0x1a);
    m_pSIM900->Write('\0');
    // check response
    if (m_pSIM900->WaitResp(10000, 10, "SEND OK") == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "CCtrlGSM: can not start TCP connection!");
        return false;
    }

    usleep(50000);

    // check if output data are required
    if (out != NULL) {
        // get data from server
        if (!m_pSIM900->Read(out, outLen)) return false;
    }

    return true;
}
