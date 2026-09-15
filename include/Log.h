#ifndef LOG_H
#define LOG_H

#include <Arduino.h>
#include <stdarg.h>

// Prints one formatted line. Kept over Serial.printf for two reasons: the printf
// attribute, with -Werror=format in platformio.ini, checks every format against
// its arguments at build time, and the 160-byte buffer holds lines past
// Serial.printf's 80-byte limit.
__attribute__((format(printf, 1, 2)))
inline void logLine(const char* fmt, ...) {
    char buffer[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    Serial.println(buffer);
}

#endif
