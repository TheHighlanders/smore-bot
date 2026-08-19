#ifndef PRINT_H
#define PRINT_H

#include <stdarg.h>
#include <stdio.h>
#include "Arduino.h"

// Text Styles / Reset
#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_UNDERLINE "\033[4m"

// Standard Foreground Colors (30-37)
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

// Bright/High-Intensity Foreground Colors (90-97)
#define BRIGHT_BLACK "\033[90m"
#define BRIGHT_RED "\033[91m"
#define BRIGHT_GREEN "\033[92m"
#define BRIGHT_YELLOW "\033[93m"
#define BRIGHT_BLUE "\033[94m"
#define BRIGHT_MAGENTA "\033[95m"
#define BRIGHT_CYAN "\033[96m"
#define BRIGHT_WHITE "\033[97m"

// Standard Background Colors (40-47)
#define BG_BLACK "\033[41m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN "\033[46m"
#define BG_WHITE "\033[47m"

// Bright/High-Intensity Background Colors (100-107)
#define BG_BRIGHT_BLACK "\033[100m"
#define BG_BRIGHT_RED "\033[101m"
#define BG_BRIGHT_GREEN "\033[102m"
#define BG_BRIGHT_YELLOW "\033[103m"
#define BG_BRIGHT_BLUE "\033[104m"
#define BG_BRIGHT_MAGENTA "\033[105m"
#define BG_BRIGHT_CYAN "\033[106m"
#define BG_BRIGHT_WHITE "\033[107m"

inline void printColor(const char* colorCode, const char* text, va_list args) {
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), text, args);
    Serial.printf("%s%s\r\n", colorCode, buffer);
    Serial.printf(ANSI_RESET);
};

inline void logUpdate(const char* text, ...) {
    va_list args;
    va_start(args, text);

    printColor(YELLOW, text, args);
    va_end(args);
};  // Prints an update (Yellow)

inline void logError(const char* text, ...) {
    va_list args;
    va_start(args, text);

    printColor(RED, text, args);
    va_end(args);
}  // Prints an error (Red);

inline void logInfo(const char* text, ...) {
    va_list args;
    va_start(args, text);

    printColor(BLUE, text, args);
    va_end(args);
}  // Prints an info message (Blue)

#endif