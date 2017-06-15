#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

// std
#include <iostream>
#include <sstream>
#include <string>

// get array size
#define arraysize(arr) (sizeof(arr)/sizeof((arr)[0]))

#define TO_STRING_(s) #s
#define TO_STRING(s) TO_STRING_(s)

#define assert(expr) ((expr)?((void)0):(void)fputs(TO_STRING(expr) " " __FILE__ ":" TO_STRING(__LINE__) "\n",stdout))
#define verify(expr) assert(expr)

// convert to std::string
template <typename T>
static std::string ToString(const T& n) { std::stringstream ss; ss << n; return ss.str(); }

// convert to ASCII
template <typename T>
static char ToASCII(const T& n) { static const char a[] = " .:-=0123456789X"; return a[(size_t(n * arraysize(a) -1))]; }

// get time from start in [ms]
unsigned GetTimeMSec();
// get time from start in [s]
double GetTimeSec();

#endif // COMMON_H
