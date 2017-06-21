#####################################
cmake_minimum_required (VERSION 2.8)
project (cam-system)

#configure_file (
#    ${CMAKE_CURRENT_BINARY_DIR}/config/cam-system.cfg
#)

# Set directories for headers and libraries
include_directories(
  src
  src/simpleini
)

# Define source files
set(cam-system_SOURCES
  src/common.cpp
  src/logger.cpp
  src/settings.cpp
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
  src/simpleini/ConvertUTF.h
  src/simpleini/SimpleIni.h
  src/settings.h
  src/ds18b20.h
  src/picture.h
  src/gpio.h
  src/serial.h
  src/sim900.h
)

set(raspicam_DIR "${ROOTFS}/usr/local/lib/cmake")
set(BINARY_NAME "cam-system")
find_package(raspicam REQUIRED)
find_package(OpenCV)
if( OpenCV_FOUND AND raspicam_CV_FOUND)
  add_executable (${BINARY_NAME} ${cam-system_SOURCES} ${cam-system_HEADERS})

  target_link_libraries (${BINARY_NAME} ${raspicam_CV_LIBS})

  # install
  #set(CMAKE_INSTALL_PREFIX ${ROOTFS}/opt/cam-system)
#  install(TARGETS ${BINARY_NAME} RUNTIME DESTINATION ${ROOTFS}/opt/cam-system)
#  install(PROGRAMS config/${BINARY_NAME} DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/init.d)
#  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/config/${BINARY_NAME}.cfg DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})
else()
  message(FATAL_ERROR "OPENCV NOT FOUND IN YOUR SYSTEM")
endif()
#####################################
