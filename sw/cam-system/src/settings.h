#ifndef SETTINGS_H
#define SETTINGS_H

#include "serial.h"
#include "logger.h"

// configuration file location
#define CFG_FILE "/opt/cam-system/cam-system.cfg"
// log file location
#define LOG_FILE "/opt/tmp/cam-system/file.log"
// system log level
#define LOG_LVL_SYSTEM LL_DEBUG
// file log level
#define LOG_LVL_FILE LL_INFO

// gprs settings
#define GPRS_APN        ""
#define GPRS_USERNAME   ""
#define GPRS_PASSWORD   ""

// server settings
#define SERVER_URL  "google.com"
#define SERVER_PORT 80
#define SERVER_PATH "/"

// sim settings
#define SIM_PIN ""

// sim900 settings
#define SIM900_BAUDRATE BR_9600

// system settings
#define SYS_NAME "RPI_cam-system"

// ini file section
#define SECTION_INI_GENERAL "GENERAL"
#define SECTION_INI_GPRS    "GPRS"
#define SECTION_INI_SERVER  "SERVER"
#define SECTION_INI_MODULE  "MODULE"

struct stSetGPRS {
    char *apn;
    char *username;
    char *password;
};

struct stSetServer {
    char *url;
    int port;
    char *path;
};

struct stSetModule {
    char *node;
    BaudRate baudrate;
    char *pin;
};

struct stSettings {
    char *pCfgFileName;
    char *pLogFileName;
    LogLevel logLevelFile;
    LogLevel logLevelSystem;
    stSetGPRS GPRS;
    stSetServer server;
    stSetModule module;
};

extern struct stSettings settings;

int LoadSettings(const char *fileName);
void UnloadSettings(void);

#endif // SETTINGS_H
