#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define CAM_SYSTEM_PATH "/opt/pi/"
#define CAM_SYSTEM_APP CAM_SYSTEM_PATH "bin/cam-system"
#define CAM_SYSTEM_CFG " -c " CAM_SYSTEM_PATH "etc/cam-system/cam-system.cfg"

#endif // COMMON_H_
