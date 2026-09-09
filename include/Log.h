#ifndef LOG_H
#define LOG_H

#include <Arduino.h>
#include <stdarg.h>

// Colored serial logging, by tier:
//   logInfo    routine per-station events
//   logUpdate  machine-level state changes
//   logError   faults and refusals
//
// The printf attribute turns format/argument mismatches (notably passing a
// std::string to %s) into build failures; platformio.ini sets -Werror=format.

#define ANSI_RESET "\033[0m"
#define ANSI_RED "\033[31m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_BLUE "\033[34m"

// Serial.printf would truncate at 80 bytes and could cut off the reset code,
// leaving the terminal stuck in color.
inline void logWrite(const char* color, const char* fmt, va_list args) {
    char buffer[160];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    Serial.print(color);
    Serial.print(buffer);
    Serial.println(ANSI_RESET);
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
