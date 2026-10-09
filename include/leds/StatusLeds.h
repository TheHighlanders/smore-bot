#ifndef STATUSLEDS_H
#define STATUSLEDS_H

#include <Adafruit_NeoPixel.h>
#include <stdint.h>

#include <vector>

#include "machine/Station.h"

struct Rgb {
    uint8_t r, g, b;
};

// Shows each station's phase on one LED strip. A zone is a station and the LEDs
// it lights. Zones draw in order, so a later zone covers an earlier one. Config.h
// sets how each phase looks.
class StatusLeds {
   public:
    struct Config {
        Rgb arriving;         // Band color
        Rgb clearing;         // Band color
        uint16_t bandWidth;   // LEDs in a band
        uint32_t bandStepMs;  // Time for a band to move one LED
    };

    struct Pulse {
        Rgb color;
        uint32_t periodMs;  // One full pulse, dim to bright to dim
    };

    struct Range {
        uint16_t first;
        uint16_t count;
    };

    struct Zone {
        const Station* station;
        Rgb idle;  // Color when idle
        Pulse pulse;
        Range leds;
    };

    StatusLeds(Adafruit_NeoPixel& strip, Config config, std::vector<Zone> zones)
        : m_strip(strip), m_config(config), m_zones(zones) {}

    // Draws every zone as it looks at `clock`.
    void update(uint32_t clock);

   private:
    Rgb colorAt(const Zone& zone, size_t position, uint32_t clock) const;
    bool inBand(size_t position, size_t count, uint32_t clock) const;

    Adafruit_NeoPixel& m_strip;
    Config m_config;
    std::vector<Zone> m_zones;
};

#endif
