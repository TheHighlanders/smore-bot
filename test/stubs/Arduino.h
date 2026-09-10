#pragma once

// Minimal Arduino surface so Machine and Station build on the host. Only what
// they actually touch: millis() and the two Serial calls Log.h makes.

#include <stdint.h>
#include <stdio.h>

extern uint32_t g_millis;
inline uint32_t millis() { return g_millis; }

struct HostSerial {
    bool quiet = true;
    void print(const char* text) {
        if (!quiet) fputs(text, stdout);
    }
    void println(const char* text) {
        if (!quiet) printf("%s\n", text);
    }
};

extern HostSerial Serial;
