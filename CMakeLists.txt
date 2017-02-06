#####################################
cmake_minimum_required (VERSION 2.8)
project (cam-system)

# Set directories for headers and libraries
include_directories(
  src
)

# Define source files
set(cam-system_SOURCES
  src/common.cpp
  src/logger.cpp
  src/ds18b20.cpp
  src/picture.cpp
  src/serial.cpp
  src/gpio.cpp
  src/sim900.cpp
  src/main.cpp
)
set(cam-system_HEADERS
  src/common.h
  src/logger.h
  src/ds18b20.h
  src/picture.h
  src/gpio.h
  src/serial.h
  src/sim900.h
)

set(raspicam_DIR "${ROOTFS}/usr/local/lib/cmake")
find_package(raspicam REQUIRED)
find_package(OpenCV)
IF ( OpenCV_FOUND AND raspicam_CV_FOUND)
  add_executable (cam-system ${cam-system_SOURCES} ${cam-system_HEADERS})

  target_link_libraries (cam-system ${raspicam_CV_LIBS})

  set(CMAKE_INSTALL_PREFIX ${ROOTFS}/opt/cam-system)
ELSE()
  MESSAGE(FATAL_ERROR "OPENCV NOT FOUND IN YOUR SYSTEM") 
ENDIF()
#####################################
