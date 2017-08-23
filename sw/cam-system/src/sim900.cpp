#include "common.h"
#include "logger.h"
#include "settings.h"
#include "gpio.h"
#include <time.h>

#include "sim900.h"

CGSM::CGSM() {
    m_pSerial = NULL;
    memset(m_commBuff, 0x00, sizeof(m_commBuff));
    m_commStatus = CLS_FREE;
    m_GSMStatus = GSM_ST_IDLE;
    m_transpMode = false;
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

int CGSM::SendATCmd(const char *ATcmd, unsigned long tmt,
    unsigned long maxCharsTmt, const char *expStr, unsigned char repeatCount) {
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

void CGSM::_Write(const char *data) {
    // check data to write
    if (data == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "data to write are missing!");
        return;
    }
    // write data to device
    m_pSerial->Puts(data, strlen(data));
}

void CGSM::_WriteLn(const char *data) {
    // check data to write
    if (data == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "data to write are missing!");
        return;
    }

    // write data to device
    m_pSerial->Puts(data, strlen(data));
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

    while (m_pSerial->DataAvailable() && (idx < len - 1)) {
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
    switch(params) {
        case INIT_PARAM_SET_0:
            SetCommStatus(CLS_ATCMD);

            // reset to the factory settings
            // TODO: is it necessary?
            //SendATCmd("AT&F", 1000, 50, STR_OK, 5);

            // switch off echo
            SendATCmd("ATE0", 500, 50, STR_OK, 5);

            SetCommStatus(CLS_FREE);
            break;

        case INIT_PARAM_SET_1:
            SetCommStatus(CLS_ATCMD);

            // request for enable clip
            SendATCmd("AT+CLIP=1", 500, 50, STR_OK, 5);
            // mobile equipment code
            SendATCmd("AT+CMEE=0", 500, 50, STR_OK, 5);
            // set SMS mode to text
            SendATCmd("AT+CMGF=1", 500, 50, STR_OK, 5);

            SetCommStatus(CLS_FREE);
            break;
    }
}

int CGSM::WaitResp(unsigned long tmt, unsigned long maxCharsTmt, const char *expStr) {
    unsigned int status = RX_ST_NOT_FINISHED;
    bool rxStarted = false;

    size_t recDataCount;
    unsigned int dataToProcess = 0;

    char *pCommBuff = m_commBuff;

    // get start time
    unsigned long tsStart = GetTimeMSec();

    m_commBuff[0] = '\0';

    // clear rx buffer
    m_pSerial->Flush();

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
            	//CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%lu", (unsigned long)(GetTimeMSec() - tsStart));

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
    } while (status == RX_ST_NOT_FINISHED);

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

    // prevent module to go to sleep mode
    SendATCmd("AT+CSCLK=0", 500, 100, STR_OK STR_CRLF, 5);

    // if no-reply we turn to turn on the module
    for (retry = 0; retry < 3; retry++) {
        if ((SendATCmd(STR_AT, 500, 100, STR_OK STR_CRLF, 5) == AT_RESP_NO_RESP) && (!turnedON)) {
            CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): No response", __COMPACT_PRETTY_FUNCTION__);

            // turn on module
            gpioON.SetValue(true);
            sleep(2);
            gpioON.SetValue(false);
            sleep(10);
            noResp = true;
            // wait for response
            WaitResp(1000, 1000);
        } else {
            CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): next step to init device", __COMPACT_PRETTY_FUNCTION__);
            noResp = false;
            // wait for response
            WaitResp(1000, 1000);
            // TODO: break ?
            break;
        }
    }

    // check if device is started
    if (SendATCmd(STR_AT, 500, 100, STR_OK STR_CRLF, 5)) {
        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): started", __COMPACT_PRETTY_FUNCTION__);
        turnedON = true;
    }

    // check started status
    if (retry == 3 && noResp) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): does not answer", __COMPACT_PRETTY_FUNCTION__);

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
        Echo(true); // TODO: change to false?

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
        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): IMEI:%s", __COMPACT_PRETTY_FUNCTION__, CompactString(ret).c_str());
    }

    return ret;
}

std::string CSIM900::GetCCI() {
    int status;
    std::string ret = std::string();

    // check status
    if (GetCommStatus() != GSM_ST_READY) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): GSM status needs to be ready!", __COMPACT_PRETTY_FUNCTION__);
        return ret;
    }

    // write AT command to get CCI
    WriteLn("AT+QCCID");

    // get CCI
    if ((status = WaitResp(5000, 50, STR_OK)) != RX_ST_FINISHED_STR_ERR) {
        // get data from device
        ret = GetCommBuff();
        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): CCI:%s", __COMPACT_PRETTY_FUNCTION__, CompactString(ret).c_str());
    }

    return ret;
}

std::string CSIM900::GetIP() {
    int status;
    std::string ret = std::string();

    // check status
    if (GetGSMStatus() != GSM_ST_ATTACHED) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): GSM status needs to be attached!", __COMPACT_PRETTY_FUNCTION__);
        return ret;
    }

    // write AT command to get IP address
    WriteLn("AT+CIFSR");

    // get CCI
    if ((status = WaitResp(5000, 50, STR_OK)) != RX_ST_FINISHED_STR_ERR) {
        // get data from device
        ret = GetCommBuff();
        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): IP:%s", __COMPACT_PRETTY_FUNCTION__, CompactString(ret).c_str());
    }

    return ret;
}

CCtrlGSM::CCtrlGSM() {
    m_pSIM900 = NULL;
    m_initialized = false;
    m_connected = false;
}

CCtrlGSM::~CCtrlGSM() {
    if (m_pSIM900 != NULL) {
        DetachGPRS();
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

bool CCtrlGSM::AttachGPRS(const char *apn, const char *user, const char *pwd, bool transpMode) {
    if (!m_initialized) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "GSM module is not initialized!");
        return false;
    }

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): establishing connection", __COMPACT_PRETTY_FUNCTION__);

    // some delay
    sleep(5);

    // wait for response
    m_pSIM900->WaitResp(50, 50);

    // set connection mode
    if (transpMode) m_pSIM900->WriteLn("AT+CIPMODE=1");
    else m_pSIM900->WriteLn("AT+CIPMODE=0");
    // check response
    if (m_pSIM900->WaitResp(500, 50, STR_OK) == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not set non/transparent mode!", __COMPACT_PRETTY_FUNCTION__);

        m_connected = false;
        return m_connected;
    }

    // set selected mode
    m_pSIM900->SetTranspMode(transpMode);

    // connect to GPRS (1-attach)
    m_pSIM900->WriteLn("AT+CGATT=1");
    // check response
    if (m_pSIM900->WaitResp(5000, 50, STR_OK) == RX_ST_FINISHED_STR_ERR) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not attach to GPRS!", __COMPACT_PRETTY_FUNCTION__);

        m_connected = false;
        return m_connected;
    }

    // get local IP address
    m_pSIM900->WriteLn("AT+CIFSR");

    // wait and check answer
    if (m_pSIM900->WaitResp(5000, 50, STR_ERR) != RX_ST_FINISHED_STR_OK) {
        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): Already have an IP", __COMPACT_PRETTY_FUNCTION__);

        // close connection
        m_pSIM900->WriteLn("AT+CIPCLOSE");
        m_pSIM900->WaitResp(50, 50);
        sleep(2);

        // de-configure module as server
        m_pSIM900->WriteLn("AT+CIPSERVER=0");
        m_pSIM900->WaitResp(5000, 50, STR_ERR);

        m_connected = false;
    } else {
        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): creating new connection", __COMPACT_PRETTY_FUNCTION__);

        // close previous connection
        m_pSIM900->WriteLn("AT+CIPSHUT");

        // check response
        if (m_pSIM900->WaitResp(500, 50, "SHUT OK") == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not close connection!", __COMPACT_PRETTY_FUNCTION__);

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
        m_pSIM900->Write("\"\r");
        sleep(1);

        // check response
        if (m_pSIM900->WaitResp(500, 50, STR_OK) == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not connect to APN!", __COMPACT_PRETTY_FUNCTION__);
            m_connected = false;
            return m_connected;
        }

        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): connected to APN", __COMPACT_PRETTY_FUNCTION__);
        sleep(5);

        // create connection to with GPRS
        m_pSIM900->WriteLn("AT+CIICR");
        // check response
        if (m_pSIM900->WaitResp(10000, 50, STR_OK) == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not create connection with GPRS!", __COMPACT_PRETTY_FUNCTION__);

            m_connected = false;
            return m_connected;
        }

        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): connected with GRPS", __COMPACT_PRETTY_FUNCTION__);
        sleep(1);

        // get local IP address
        m_pSIM900->WriteLn("AT+CIFSR");
        // check response
        if (m_pSIM900->WaitResp(5000, 50, STR_ERR) != RX_ST_FINISHED_STR_OK) {
            CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): IP address was assigned", __COMPACT_PRETTY_FUNCTION__);

            // set gsm status as attached
            m_pSIM900->SetGSMStatus(GSM_ST_ATTACHED);

            m_connected = true;
            return m_connected;
        }

        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s", CompactString(m_pSIM900->GetCommBuff()).c_str());

        m_connected = false;
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): no IP address after connection!", __COMPACT_PRETTY_FUNCTION__);
    }

    return m_connected;
}

bool CCtrlGSM::DetachGPRS() {
    if (!m_initialized) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): GSM module is not initialized!", __COMPACT_PRETTY_FUNCTION__);
        return false;
    }

    // check status
    if (m_pSIM900->GetGSMStatus() == GSM_ST_IDLE) return true;

    // disconnect from TCP connection
    DisconnectTCP();

    // close previous connection
    m_pSIM900->WriteLn("AT+CIPSHUT");
    // check response
    if (m_pSIM900->WaitResp(500, 50, "SHUT OK") == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not close connection!", __COMPACT_PRETTY_FUNCTION__);

        // set error status
        m_pSIM900->SetGSMStatus(GSM_ST_ERROR);
        return false;
    }

    // disconnect from GPRS (0-detach)
    m_pSIM900->WriteLn("AT+CGATT=0");
    // check response
    if (m_pSIM900->WaitResp(5000, 50, STR_OK) == RX_ST_FINISHED_STR_ERR) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not close connection!", __COMPACT_PRETTY_FUNCTION__);

        // set error status
        m_pSIM900->SetGSMStatus(GSM_ST_ERROR);
        return false;
    }

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): connection was successfully closed!", __COMPACT_PRETTY_FUNCTION__);

    // set status as disconnect
    m_pSIM900->SetGSMStatus(GSM_ST_READY);
    m_connected = false;
    return true;
}

bool CCtrlGSM::ConnectTCP(const char *server, unsigned int port) {
    // check if is connected
    if (!m_connected) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): no GPRS connection!", __COMPACT_PRETTY_FUNCTION__);
        return false;
    }

    // start tcp connection
    m_pSIM900->Write("AT+CIPSTART=\"TCP\",\"");
    m_pSIM900->Write(server);
    m_pSIM900->Write("\",");
    m_pSIM900->WriteLn(ToString(port).c_str());
    sleep(1);
    // check response
    if (m_pSIM900->WaitResp(1000, 200, STR_OK) == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not start TCP connection!", __COMPACT_PRETTY_FUNCTION__);
        return false;
    }

    // get last response
    const std::string str = m_pSIM900->GetCommBuff();
    // TODO: only for test
    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "resp:%s", CompactString(str).c_str());

    // check connection status
    if (!str.compare(2, 10, "CONNECT OK")) {
        if (m_pSIM900->WaitResp(15000, 200, STR_OK) == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not connect to server!", __COMPACT_PRETTY_FUNCTION__);
            CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): server response: %s", __COMPACT_PRETTY_FUNCTION__, str.c_str());
            return false;
        }
    }

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): connected to server: %s", __COMPACT_PRETTY_FUNCTION__, server);
    sleep(3);

    // CIPSEND is available only in non-transparent mode
    if (!m_pSIM900->GetTranspMode()) {
        // open connection for data sending
        m_pSIM900->WriteLn("AT+CIPSEND");
        if (m_pSIM900->WaitResp(5000, 200, ">") == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not open connection for data sending!", __COMPACT_PRETTY_FUNCTION__);
            m_connected = false;
            return m_connected;
        }
    }

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): opened connection for data sending", __COMPACT_PRETTY_FUNCTION__);
    sleep(4);

    return true;
}

bool CCtrlGSM::DisconnectTCP() {
    // check if is connected
    if (!m_connected) {
        CLogger::GetLogger()->LogPrintf(LL_INFO, "%s(): no GPRS connection!", __COMPACT_PRETTY_FUNCTION__);
        return false;
    }

    // disconnect from server
    if (m_pSIM900->GetTranspMode()) m_pSIM900->WriteLn("ATH");

    m_pSIM900->WriteLn("AT+CIPCLOSE");

    // status handling
    if (m_pSIM900->GetGSMStatus() == GSM_ST_TCP_CLIENT_CONNECTED)
        m_pSIM900->SetGSMStatus(GSM_ST_ATTACHED);
    else m_pSIM900->SetGSMStatus(GSM_ST_TCP_SERVER_WAIT);

    return true;
}

bool CCtrlGSM::HttpGET(const char *server, unsigned int port, const char *url, char *pOut, size_t outLen) {
    bool connected = false;

    // check server and output
    if (server == NULL || url == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): server or path for connection are missing!", __COMPACT_PRETTY_FUNCTION__);
        return false;
    }

    for (int retry = 0; retry < 3; retry++) {
        // try to connect to server
        if (ConnectTCP(server, port)) {
            CLogger::GetLogger()->LogPrintf(LL_INFO,"%s(): connected to server %s", __COMPACT_PRETTY_FUNCTION__, server);
            connected = true;
            break;
        }
        CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): connecting....%i", __COMPACT_PRETTY_FUNCTION__, retry);
    }

    // check connection status
    if (!connected) {
        CLogger::GetLogger()->LogPrintf(LL_INFO, "%s(): not connected to server %s", __COMPACT_PRETTY_FUNCTION__, server);
        return false;
    }

    // write data to server
    m_pSIM900->Write("GET ");
    m_pSIM900->Write(url);
    m_pSIM900->Write(" HTTP/1.0\r\nHost: ");
    m_pSIM900->Write(server);
    m_pSIM900->Write(STR_CRLF);
    m_pSIM900->Write("User-Agent: ");
    m_pSIM900->Write(SYS_NAME);
    m_pSIM900->Write(STR_CRLF STR_CRLF);
    m_pSIM900->Write(STR_CTRLZ);
    m_pSIM900->Write('\0');

    sleep(1);

    // check response
    if (m_pSIM900->WaitResp(10000, 10, "SEND OK") == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not send data over GET method!", __COMPACT_PRETTY_FUNCTION__);
        return false;
    }

    usleep(50000);

    // check if output data are required
    if (pOut != NULL) {
        // get data from server
        if (!m_pSIM900->Read(pOut, outLen)) return false;
    }

    return true;
}

bool CCtrlGSM::HttpPOST(const char *server, unsigned int port, const char *url, const char *params, char *pOut, size_t outLen) {
    bool connected = false;

    // check server and output
    if (server == NULL || url == NULL || params == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): server, path or parameters for connection are missing!", __COMPACT_PRETTY_FUNCTION__);
        return false;
    }

    // check if is not already connected
    m_pSIM900->WriteLn("AT+CIPSTATUS");
    // check response
    if (m_pSIM900->WaitResp(500, 50, "STATE: CONNECT OK") == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
        connected = true;
    }

    if (!connected) {
        for (int retry = 0; retry < 3; retry++) {
            // try to connect to server
            if (ConnectTCP(server, port)) {
                CLogger::GetLogger()->LogPrintf(LL_INFO,"%s(): connected to server %s", __COMPACT_PRETTY_FUNCTION__, server);
                connected = true;
                break;
            }
            CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): connecting....%i", __COMPACT_PRETTY_FUNCTION__, retry);
        }
    } else {
        // CIPSEND is available only in non-transparent mode
        if (!m_pSIM900->GetTranspMode()) {
            m_pSIM900->WriteLn("AT+CIPSEND");
            if (m_pSIM900->WaitResp(5000, 200, ">") == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
                CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not open connection for data sending!", __COMPACT_PRETTY_FUNCTION__);
                return false;
            }
        } else {
            m_pSIM900->WriteLn("ATO");
            if (m_pSIM900->WaitResp(5000, 200, "CONNECT") == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
                CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not open connection for data sending!", __COMPACT_PRETTY_FUNCTION__);
                return false;
            }
        }
    }

    // check connection status
    if (!connected) {
        CLogger::GetLogger()->LogPrintf(LL_INFO, "%s(): not connected to server %s", __COMPACT_PRETTY_FUNCTION__, server);
        return false;
    }

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): start sending data", __COMPACT_PRETTY_FUNCTION__, server);

    // write data to server
    m_pSIM900->Write("POST ");
    m_pSIM900->Write(url);
    m_pSIM900->Write(" HTTP/1.1\r\nHost: ");
    m_pSIM900->Write(server);
    m_pSIM900->Write(STR_CRLF);
    m_pSIM900->Write("User-Agent: ");
    m_pSIM900->WriteLn(SYS_NAME);
    m_pSIM900->WriteLn("Content-Type: application/x-www-form-urlencoded");
    m_pSIM900->WriteLn("Expect:  ");
    m_pSIM900->Write("Content-Length: ");
    m_pSIM900->WriteLn(ToString(strlen(params)).c_str());
    m_pSIM900->Write(STR_CRLF);
    m_pSIM900->Write(params);
    m_pSIM900->Write(STR_CRLF STR_CRLF);

    if (!m_pSIM900->GetTranspMode()) m_pSIM900->Write(STR_CTRLZ);
    else {
        // wait before switching to command mode
        sleep(1);
        m_pSIM900->Write("+++");
        sleep(1);
    }

    m_pSIM900->Write('\0');

    // check response
    if (!m_pSIM900->GetTranspMode()) {
        if (m_pSIM900->WaitResp(10000, 100, "SEND OK") == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not send data over POST method!", __COMPACT_PRETTY_FUNCTION__);
            return false;
        }
    } else {
        if (m_pSIM900->WaitResp(10000, 100, "OK") == (RX_ST_TIMEOUT_ERR || RX_ST_FINISHED_STR_ERR)) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not send data over POST method!", __COMPACT_PRETTY_FUNCTION__);
            return false;
        }
    }

    CLogger::GetLogger()->LogPrintf(LL_INFO, "%s(): data have been successfully sent", __COMPACT_PRETTY_FUNCTION__, server);

    usleep(50000);

    // check if output data are required
    if (pOut != NULL) {
        // get data from server
        if (!m_pSIM900->Read(pOut, outLen)) return false;
    }

    return true;
}

bool CCtrlGSM::SendSMS(const char *number, const char *msg) {
    // check number and message
    if (number == NULL || msg == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): number or message test to send SMS are missing!", __COMPACT_PRETTY_FUNCTION__);
        return false;
    }

    // check message text length
    if (strlen(msg) > SMS_MAX_LEN) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not send message longer than %s characters!",__COMPACT_PRETTY_FUNCTION__, SMS_MAX_LEN);
        return false;
    }

    if (!m_initialized) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): GSM module is not initialized!", __COMPACT_PRETTY_FUNCTION__);
        return false;
    }

    // check link status
    if (m_pSIM900->GetGSMStatus() != GSM_ST_IDLE) {
        CLogger::GetLogger()->LogPrintf(LL_WARNING, "%s(): GSM status is not IDLE", __COMPACT_PRETTY_FUNCTION__);
        return false;
    }

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "%s(): trying to send SMS to number %s", __COMPACT_PRETTY_FUNCTION__, number);

    bool sent = false;

    // try to send sms 3-times
    for (int i = 0; i < 3; i++) {
        // set sms number
        m_pSIM900->Write("AT+CMGS=\"");
        m_pSIM900->Write(number);
        m_pSIM900->WriteLn("\"");

        // check response
        if (m_pSIM900->WaitResp(1000, 500, ">") == RX_ST_FINISHED_STR_OK) {
            // set sms text
            m_pSIM900->Write(msg);
            m_pSIM900->Write(STR_CTRLZ);
            m_pSIM900->Write('\0');

            // check if message was sent
            if (m_pSIM900->WaitResp(7000, 5000, "+CMGS")) {
                sent = true;
                break;
            }
        }
    }

    if (sent) CLogger::GetLogger()->LogPrintf(LL_INFO, "%s(): SMS was sent successfully", __COMPACT_PRETTY_FUNCTION__);
    else CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): SMS was not sent!", __COMPACT_PRETTY_FUNCTION__);

    // set comm status
    m_pSIM900->SetCommStatus(CLS_FREE);

    return sent;
}

int CCtrlGSM::GetSMS(unsigned char position, char *number, char *pMsg, size_t outlen) {
    int ret = -1;

    // check output number and buffer for text
    if (number == NULL || pMsg == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): number or message test to send SMS are missing!", __COMPACT_PRETTY_FUNCTION__);
        return false;
    }

    // check position
    if (!position) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): can not access to 0 SMS position!", __COMPACT_PRETTY_FUNCTION__);
        return ret;
    }

    if (!m_initialized) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "%s(): GSM module is not initialized!", __COMPACT_PRETTY_FUNCTION__);
        return ret;
    }

    // check comm status
    if (m_pSIM900->GetCommStatus() != CLS_FREE) {
        CLogger::GetLogger()->LogPrintf(LL_WARNING, "%s(): CommLine status is not FREE", __COMPACT_PRETTY_FUNCTION__);
        return ret;
    }
    // set comm status
    m_pSIM900->SetCommStatus(CLS_ATCMD);

    // set output to no SMS
    ret = SMS_ST_NO_SMS;

    // get sms on position
    m_pSIM900->Write("AT+CMGR=");
    m_pSIM900->WriteLn(position);

    // get response
    int resp = m_pSIM900->WaitResp(5000, 100, "+CMGR");

    // get last response
    const std::string str = m_pSIM900->GetCommBuff();

    // switch by response
    switch (resp) {
        case RX_ST_TIMEOUT_ERR:
            // response timeout
            ret = -2;
            break;

        case RX_ST_FINISHED_STR_ERR:
            // SMS was received, but not on entered position
            if (!str.compare(STR_OK)) {
                ret = SMS_ST_NO_SMS;
            } else if (!str.compare(STR_ERR)) {
                ret = SMS_ST_NO_SMS;
            }
            break;

        case RX_ST_FINISHED_STR_OK:
            // check received sms
            if (!str.compare("\"REC UNREAD\"")) {
                // set status for unread sms
                ret = SMS_ST_UNREAD;
            } else if (!str.compare("\"REC READ\"")) {
                // get phone number from received sms
                ret = SMS_ST_READ;
            } else {
                // other sms was found
                ret = SMS_ST_OTHER;
            }

            // extract number from sms
            // TODO: number parser

            // copy text to output buffer
            // TODO: text parser
            break;
    }

    // set comm status
    m_pSIM900->SetCommStatus(CLS_FREE);
    return ret;
}
