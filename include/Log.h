#ifndef LOG_H
#define LOG_H

#include <Arduino.h>
#include <stdarg.h>

// Colored serial logging. The printf attribute makes format/argument mismatches
// (notably passing a std::string to %s) a compile error rather than a crash.

#define ANSI_RESET "\033[0m"
#define ANSI_RED "\033[31m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"

inline void logWrite(const char* color, const char* fmt, va_list args) {
    char buffer[160];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    Serial.printf("%s%s" ANSI_RESET "\r\n", color, buffer);
}

__attribute__((format(printf, 1, 2)))
inline void logInfo(const char* fmt, ...) {
    va_list args; va_start(args, fmt); logWrite(ANSI_BLUE, fmt, args); va_end(args);
}

__attribute__((format(printf, 1, 2)))
inline void logUpdate(const char* fmt, ...) {
    va_list args; va_start(args, fmt); logWrite(ANSI_YELLOW, fmt, args); va_end(args);
}

__attribute__((format(printf, 1, 2)))
inline void logError(const char* fmt, ...) {
    va_list args; va_start(args, fmt); logWrite(ANSI_RED, fmt, args); va_end(args);
}

#endif
