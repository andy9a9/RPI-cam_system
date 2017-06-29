#include "common.h"
#include <stdarg.h>
#include <sys/time.h>

#include "logger.h"

CLogger* CLogger::m_pThis = NULL;
char CLogger::m_logBuffer[MAX_MSG_LEN] = {};

const char *LevelStr[] = {
    "",
    "",
    "",
    "error",
    "warning",
    "info",
    "debug"
};

CLogger::CLogger() :
    m_systemLogLevel(LL_DEBUG),
    m_fileLogLevel(LL_INFO),
    m_logFile(NULL),
    m_logTime(true),
    m_appendNewLine(true),
    m_pLogPrefix("cam-system") {
}

CLogger::~CLogger() {
    CloseLogFile();
    m_pLogPrefix = NULL;
    m_pThis = NULL;
}

CLogger* CLogger::GetLogger() {
    // create new logger if not exist
    if (m_pThis == NULL) m_pThis = new CLogger();
    return m_pThis;
}

bool CLogger::OpenLogFile(const LogLevel loglvl, const char *fileName) {
    if (m_logFile != NULL) return false;

    // check file path length
    if (fileName == NULL || (strlen(fileName) >= MAX_FILE_PATH_LEN - 1)) {
        std::cerr << "Error: log file path " << fileName << " is missing or is too long!" << std::endl;
        return false;
    }

    // open file
    if ((m_logFile = fopen(fileName, "w+")) == NULL) {
        std::cerr << "Error: can not open file " << fileName << " for writing!" << std::endl;
        return false;
    }

    return true;
}

bool CLogger::CloseLogFile() {
    if (m_logFile == NULL) return false;

    fclose(m_logFile);
    m_logFile = NULL;
    return true;
}

void CLogger::SetLogPrefix(const char *prefix) {
    if (prefix != NULL) m_pLogPrefix = prefix;
}

int CLogger::PrependPrefix(const LogLevel loglvl, char *pMsg) {
    // check if pMsg is not null
    if (pMsg == NULL) return 0;

    struct tm timeinfo;

    // check if time needs to be logged
    if (m_logTime) {
        // get actual time
        time_t now = time(NULL);
        // get localtime
        localtime_r(&now, &timeinfo);

        return snprintf(pMsg, MAX_MSG_LEN, "[%s] %7s: %02d.%02d.%04d %02d:%02d:%02d : ",
            m_pLogPrefix, LevelStr[loglvl],
            timeinfo.tm_mday, timeinfo.tm_mon, timeinfo.tm_year + 1900,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    }
    return snprintf(pMsg, MAX_MSG_LEN, "[%s] %7s: ", m_pLogPrefix, LevelStr[loglvl]);
}

void CLogger::LogPuts(char *pOutput, size_t len) {
    // append new line if enabled
    if (m_appendNewLine) {
        *(pOutput + len - 1) = '\n';
        *(pOutput + len) = '\0';
    }

    // log message to file
    if (m_logFile != NULL) {
        if (fwrite(pOutput, len, 1, m_logFile) == 1) {
            fflush(m_logFile);
        } else fprintf(stderr, "Error: failed to write to logfile!\n");
    }

    // log message to output
    if (m_systemLogLevel != 0) {
        fprintf(stdout, "%s", pOutput);
        fflush(stdout);
    }
}

bool CLogger::LogPrintf(const LogLevel loglvl, const char *message, ...) {
    if ((loglvl) && ((loglvl <= m_fileLogLevel) || (loglvl <= m_systemLogLevel))) {
        int size;
        int prefLen;
        char *msg = NULL;

        va_list args;
        // format string with message
        va_start(args, message);

        // prepend time and prefix
        if ((prefLen = PrependPrefix(loglvl, m_logBuffer)) == 0) {
            std::cerr << "Error: can not prepend prefix to file!" << std::endl;
            return false;
        }

        // get formated string length
        size = vsnprintf(NULL, 0, message, args) + 1;
        // check max size
        if (size > (MAX_MSG_LEN - prefLen + 1)) {
            std::cerr << "Error: message is too long!" << std::endl;
            return false;
        }

        // allocate new size
        msg = new char[size];

        // format message
        size = vsnprintf(msg, size, message, args);
        va_end(args);

        // append message to output buffer
        strcat(m_logBuffer, msg);

        // clean new buffer
        free(msg);
        msg = NULL;

        // log message
        LogPuts(m_logBuffer, prefLen + size + 1);
    }
    return true;
}
