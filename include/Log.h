#ifndef LOG_H
#define LOG_H

#include <Arduino.h>
#include <stdarg.h>

// One line to the serial monitor.
//
// The printf attribute makes a format that does not match its arguments a
// build error; platformio.ini sets -Werror=format. Serial.printf truncates at
// 80 bytes, so the line is built first and printed whole.
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
