#include <time.h>
#include <stdarg.h>

#include "common.h"

struct timespec c_TimeRes;
struct timespec c_TimeStart;

static class CPreciseTime
{
public:
    CPreciseTime() {
        clock_getres(CLOCK_MONOTONIC, &c_TimeRes);
        clock_gettime(CLOCK_MONOTONIC, &c_TimeStart);
    }
} c_initCPreciseTime;

unsigned GetTimeMSec() {
    struct timespec act;

    clock_gettime(CLOCK_MONOTONIC, &act);

    long ret = act.tv_sec - c_TimeStart.tv_sec;
    ret *= 1000; // [s]->[ms]
    ret += act.tv_nsec / 1000000; // [ns]->[ms]

    return ret;
}

double GetTimeSec() {
    struct timespec tm;
    clock_gettime(CLOCK_MONOTONIC, &tm);
    return ((tm.tv_sec - c_TimeStart.tv_sec) + tm.tv_nsec * 0.000000001);
}

std::string ComputeMethodName(const std::string &function, const std::string &prettyFunction) {
    size_t locFunName = prettyFunction.find(function);
    size_t begin = prettyFunction.rfind(" ",locFunName) + 1;
    size_t end = prettyFunction.find("(",locFunName + function.length());
    return prettyFunction.substr(begin,end - begin);
}

std::string ReplaceString(std::string base, const std::string from, const std::string to) {
    std::string tmpStr = base;

    for (size_t start_pos = tmpStr.find(from); start_pos != std::string::npos; start_pos = tmpStr.find(from,start_pos))
        tmpStr.replace(start_pos, from.length(), to);

    return tmpStr;
}
