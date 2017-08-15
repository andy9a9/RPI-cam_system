#####################################
cmake_minimum_required (VERSION 2.8)
project (cam-wake)

# Set directories for headers and libraries
include_directories(
  src
)

# Define source files
set(cam-wake_SOURCES
  src/common.c
  src/gpio.c
  src/interrupts.c
  src/main.c
)
set(cam-wake_HEADERS
  src/common.h
  src/gpio.h
  src/interrupts.h
)

set(BINARY_NAME "cam-wake")
add_executable (${BINARY_NAME} ${cam-wake_SOURCES} ${cam-wake_HEADERS})
target_link_libraries (${BINARY_NAME} pthread)

#####################################
