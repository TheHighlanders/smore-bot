#ifndef LEDSTRIP_H
#define LEDSTRIP_H

#include <stdint.h>

struct Rgb {
    uint8_t r, g, b;
};

// A row of LEDs. set() picks an LED's color and ignores an index past the end.
// show() lights them all.
class LedStrip {
   public:
    virtual ~LedStrip() {}

    virtual void set(uint16_t index, Rgb color) = 0;
    virtual void show() = 0;
};

#endif
