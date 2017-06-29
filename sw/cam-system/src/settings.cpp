#include "common.h"
#include <sys/stat.h>
#include "SimpleIni.h"

#include "settings.h"

static char defCfgFile[] = CFG_FILE;
static char defLogFile[] = LOG_FILE;

struct stSettings settings = {
    defCfgFile,
    defLogFile,
    LOG_LVL_SYSTEM,
    LOG_LVL_FILE,
    {
        (char *)GPRS_APN,
        (char *)GPRS_USERNAME,
        (char *)GPRS_PASSWORD
    },
    {
        (char *)SERVER_URL,
        SERVER_PORT,
        (char *)SERVER_PATH
    },
    {
        (char *)DEF_SERIAL_NODE,
        SIM900_BAUDRATE,
        (char *)SIM_PIN
    }
};

bool LoadSettingsString(CSimpleIniA *pIni, char **pConfig, const char *pSectionName, const char *pKeyName) {
    const char *pStr;
    pStr = pIni->GetValue(pSectionName, pKeyName, *pConfig);
    if (pStr) {
        *pConfig = strdup(pStr);
        if (*pConfig == NULL) {
            CLogger::GetLogger()->LogPrintf(LL_ERROR, "Allocation of memory for key '%s' failed!", pKeyName);
            return false;
        }
    }
    return true;
}

int LoadSettings(const char *fileName) {
    // check if file name is not null
    if (fileName == NULL) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "Configuration file is null!");
        return -1;

    }
    // check if settings file exist with r rights
    if (access(fileName, R_OK) < 0) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "Configuration file \"%s\" is not readable!", fileName);
        return -1;
    }

    SI_Error ret;

    // read settings from file regarding to iniSettingsDesc descriptor
    CSimpleIniA ini;
    if ((ret = ini.LoadFile(fileName)) < 0) {
        CLogger::GetLogger()->LogPrintf(LL_ERROR, "Configuration file %s reading failed with error: %i!", fileName, ret);
        return ret;
    }

    // load log file
    LoadSettingsString(&ini, &settings.pLogFileName, SECTION_INI_GENERAL, "LogFileName");
    settings.logLevelFile = static_cast<LogLevel>(ini.GetLongValue(SECTION_INI_GENERAL, "LogFileLevel", LOG_LVL_FILE));
    // enable logging into file
    if (!CLogger::GetLogger()->OpenLogFile(settings.logLevelFile, settings.pLogFileName)) {
        return SI_FILE;
    }

    // gprs parameters
    LoadSettingsString(&ini, &settings.GPRS.apn, SECTION_INI_GPRS, "apn");
    LoadSettingsString(&ini, &settings.GPRS.username, SECTION_INI_GPRS, "username");
    LoadSettingsString(&ini, &settings.GPRS.password, SECTION_INI_GPRS, "password");

    // server settings
    LoadSettingsString(&ini, &settings.server.url, SECTION_INI_SERVER, "server");
    settings.server.port = ini.GetLongValue(SECTION_INI_SERVER, "port", SERVER_PORT);
    LoadSettingsString(&ini, &settings.server.path, SECTION_INI_SERVER, "path");

    // module settings
    LoadSettingsString(&ini, &settings.module.node, SECTION_INI_MODULE, "device");
    settings.module.baudrate = static_cast<BaudRate>(ini.GetLongValue(SECTION_INI_MODULE, "baudrate", SIM900_BAUDRATE));
    LoadSettingsString(&ini, &settings.module.pin, SECTION_INI_MODULE, "pin");

    CLogger::GetLogger()->LogPrintf(LL_DEBUG, "Parameters have been successfully loaded!");

    return ret;
}

void UnloadSettings(void) {
    // check cfg file
    if (settings.pCfgFileName != NULL)
        free(settings.pCfgFileName);

    // check log file
    if (settings.pLogFileName != NULL)
        free(settings.pLogFileName);

    // TODO: need test
    if (&settings.GPRS != NULL) {
        for (int i=0;i<3;i++) {
            char *tmp = (char *)(settings.GPRS.apn) + i*(sizeof(char));
            if (tmp != NULL) {
                free(tmp);
            }
        }
    }
}
