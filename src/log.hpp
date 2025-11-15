//
//  Simple printf-based logger.
//  Define the LOGOFF macro to disable all logging when using the convenience macros. Useful if you are not
//  interested at all in logging your application on a, e.g., release build
//

#pragma once

#include <cassert>

#ifndef NDEBUG
#    define LASSERT(cond, ...) if(!(cond)){Logger::log(Logger::LOG_ERROR, __VA_ARGS__); __builtin_trap();} // Not portable, only usable in linux
#else
#    define LASSERT(...) (void)0
#endif

#ifndef LOGOFF
#    define LTRACE(...) Logger::log(Logger::LOG_TRACE, __VA_ARGS__)
#    define LDEBUG(...) Logger::log(Logger::LOG_DEBUG, __VA_ARGS__)
#    define LINFO(...) Logger::log(Logger::LOG_INFO, __VA_ARGS__)
#    define LWARN(...) Logger::log(Logger::LOG_WARN, __VA_ARGS__)
#    define LERROR(...) Logger::log(Logger::LOG_ERROR, __VA_ARGS__)
#else
#    define LTRACE(...) (void)0
#    define LDEBUG(...) (void)0
#    define LINFO(...) (void)0
#    define LWARN(...) (void)0
#    define LERROR(...) (void)0
#endif

namespace Logger {

enum
{
    LOG_TRACE,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
};

void log(int level, const char* fmt, ...) __attribute__((format(printf, 2, 3)));

void set_level(int log_level);

}
