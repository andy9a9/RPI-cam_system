#ifndef LOGGER_H_
#define LOGGER_H_

#include <stdio.h>

typedef enum LogLevel {
	LL_OFF = 0, // turn off logging
	LL_ERROR = 3,
	LL_WARNING,
	LL_INFO,
	LL_DEBUG,
	LL_ALL = LL_DEBUG,
	LL_COUNT
} LogLevel;

// max length of logged massage
#define MAX_MSG_LEN 		200
// max length of logged file path
#define MAX_FILE_PATH_LEN 	250

#define LOGGER CLogger::GetLogger()

class CLogger {
public:
	static CLogger* GetLogger();
	~CLogger();

	bool OpenLogFile(const LogLevel loglvl, const char *fileName);
	bool CloseLogFile();

	bool LogPrintf(const LogLevel loglvl, const char *message, ...);

	void SetLogPrefix(const char *prefix);
	inline void SetNewLineAppend(bool enable) { m_appendNewLine = enable; }

private:
	CLogger();
	CLogger(CLogger const& copy); // not implemented
	CLogger& operator=(CLogger const& copy); // not implemented

	int PrependPrefix(const LogLevel loglvl, char *pMsg);

protected:
	void LogPuts(char *pOutput, size_t len);

private:
	static CLogger* m_pThis;
	enum LogLevel m_systemLogLevel;
	enum LogLevel m_fileLogLevel;
	FILE *m_logFile;
	bool m_logTime;
	bool m_appendNewLine;
	const char *m_pLogPrefix;
	static char m_logBuffer[MAX_MSG_LEN];
};

#endif // LOGGER_H_
