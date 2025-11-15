#include "log.hpp"

#include <cstdarg>
#include <cstdio>
#include <ctime>

#ifndef NDEBUG  // Sets the default log level
static int LogLevel = Logger::LOG_DEBUG;
#else
static int LogLevel = Logger::LOG_INFO;
#endif

const char* level_strings[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR"};

void Logger::set_level(int log_level) { LogLevel = log_level; }

void Logger::log(int level, const char* fmt, ...)
{
    if ( level >= LogLevel ) {
        std::va_list args;

        std::time_t time_epox = std::time(nullptr);
        std::tm*    time = std::localtime(&time_epox);
        char        buf[16];
        buf[strftime(buf, sizeof(buf), "%H:%M:%S", time)] = '\0';

        fprintf(stderr, "%s [%-5s]: ", buf, level_strings[level]);
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
        va_end(args);
    }
}
