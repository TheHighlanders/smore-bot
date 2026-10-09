#ifndef STATUSLEDS_H
#define STATUSLEDS_H

#include <Adafruit_NeoPixel.h>
#include <stdint.h>

#include <vector>

#include "machine/Station.h"

struct Rgb {
    uint8_t r, g, b;
};

// How a band moves across a zone.
enum class BandMotion {
    kJump,  // Starts again at the first LED
    kWrap,  // Leaves at the last LED and comes back in at the first
};

// How a working station looks.
enum class WorkLook {
    kPulse,     // Its own color pulses
    kProgress,  // Pulses while a bar fills from fillFrom to fillTo
};

// Shows each station's phase on one LED strip. A zone is a station and the LEDs
// it lights. Zones draw in order, so a later zone covers an earlier one. Config.h
// sets how each phase looks.
class StatusLeds {
   public:
    struct Config {
        Rgb runningIdle;      // Idle color while the machine runs
        Rgb arriving;         // Band color
        Rgb clearing;         // Band color
        Rgb fillFrom;         // Progress bar: LEDs not yet filled
        Rgb fillTo;           // Progress bar: filled LEDs, and the done color
        uint16_t bandWidth;   // LEDs in a band
        uint32_t bandStepMs;  // Time for a band to move one LED
        BandMotion band;      // How bands move across a zone
        WorkLook work;        // How a working station looks
    };

    struct Pulse {
        Rgb color;          // Pulse color for WorkLook::kPulse, and done in that look
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

    // Draws every zone as it looks at `clock`. `running` is whether the machine runs.
    void update(uint32_t clock, bool running);
    void selfTest();

   private:
    Rgb colorAt(const Zone& zone, size_t position, uint32_t clock, bool running) const;
    Rgb progressAt(const Zone& zone, size_t position, uint32_t clock, uint8_t progress) const;
    bool inBand(size_t position, size_t count, uint32_t clock) const;

    Adafruit_NeoPixel& m_strip;
    Config m_config;
    std::vector<Zone> m_zones;
};

#endif
