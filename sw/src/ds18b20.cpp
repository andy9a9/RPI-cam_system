#include "common.h"
#include "logger.h"
#include <fstream>
#include <dirent.h>

#include "ds18b20.h"

CDS18B20::CDS18B20() {
	m_addr = NULL;
	m_unit = UNIT_CELSIUS;
	m_devCount = 0;
	m_currIdx = 0;
	m_devices.clear();
	m_devPath = OW_PATH;
}

CDS18B20::~CDS18B20() {
	m_addr = NULL;
	m_devices.clear();
	m_devPath.clear();
}

bool CDS18B20::Init() {
	DIR *dir;
	struct dirent *drp = NULL;
	bool ret = false;

	// check directory for 1-wire devices exist
	if ((dir = opendir(m_devPath.c_str())) == NULL) {
		CLogger::GetLogger()->LogPrintf(LL_ERROR, "directory %s for one-wire is missing!", m_devPath.c_str());
		return false;
	}

	// clean actual devices
	m_devCount = 0;
	m_devices.clear();

	// read 1-wire devices
	while ((drp = readdir(dir)) != NULL) {
		// search only for DS18-devices
		if ((drp->d_type == DT_LNK) && (!strncmp(drp->d_name, DS18_PREFIX, 3))) {
			// append new devices
			m_devices.push_back(m_devPath + drp->d_name + OW_SLAVE);
			// increase device count
			m_devCount++;
		}
	}

	// make some reading for all devices
	for (std::vector<std::string>::size_type i = 0; i < m_devices.size(); i++) {
		if (GetTemp(i) == -1) {
			CLogger::GetLogger()->LogPrintf(LL_ERROR, "can not get temperature from device %s!",
					m_devices.at(i).c_str());
			ret = false;
			break;
		}
	}

	// some cleaning
	drp = NULL;
	free(drp);
	closedir(dir);

	// check device count
	if (!m_devCount) {
		CLogger::GetLogger()->LogPrintf(LL_WARNING, "no DS18B20 devices has been found!");
		ret = false;
	}

	return ret;
}

float CDS18B20::GetTemp(unsigned char idx) {
	// check index range
	if (idx > m_devCount) {
		CLogger::GetLogger()->LogPrintf(LL_ERROR, "index %i is out of range!", idx);
		return -1;
	}

	if (m_devCount == 0) {
		CLogger::GetLogger()->LogPrintf(LL_ERROR, "no DS18B20 devices has been found!", idx);
		return -1;
	}

	std::ifstream devFile(m_devices.at(idx).c_str(), std::ios::in);
	std::string line;
	bool crc = false;

	// open file for reading
	if (!devFile.is_open()) {
		CLogger::GetLogger()->LogPrintf(LL_ERROR, "can not open file %s for reading!", m_devPath.c_str());
		return -1;
	}

	try {
	    // read file line by line
	    while (std::getline(devFile, line)) {
		    // check if crc has not bee verified
		    if (!crc) {
			    if (line.substr(line.size() - 3).compare("YES")) {
				    CLogger::GetLogger()->LogPrintf(LL_ERROR, "CRC \"%s\" is not correct!", line.c_str());
				    break;
			    } else {
				    // CRC is OK
				    crc = true;
				    // go to read temperature
				    continue;
			    }
		    }

		    // read temperature
		    line = line.substr(line.find("t="));
		    // check size
		    if (!line.size()) {
			    CLogger::GetLogger()->LogPrintf(LL_ERROR, "temperature %s is missing!", line.c_str());
		    }

		    // remove t= prefix
		    line = line.erase(0,2);
	    }
	} catch (std::exception &ex) {
	    CLogger::GetLogger()->LogPrintf(LL_ERROR, "GetTemp() %s", ex.what());
	    crc = false;
	}

	// close file
	devFile.close();

	CLogger::GetLogger()->LogPrintf(LL_DEBUG, "dev: %i temp:%s", idx, line.c_str());

	// convert output to float
	return (!crc ? -1 : (atof(line.c_str()) / 1000.0));
}
