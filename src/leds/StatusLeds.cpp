#include "leds/StatusLeds.h"
#include <Log.h>

namespace {

const Rgb kOff = {0, 0, 0};

// Rises from 0 to 255 in the first half of a pulse, then falls back to 0.
uint8_t pulseLevel(uint32_t clock, uint32_t periodMs) {
    uint32_t t = clock % periodMs;
    uint32_t half = periodMs / 2;
    uint32_t distance = t <= half ? t : periodMs - t;
    return distance * 255 / half;
}

Rgb dimmed(Rgb color, uint8_t level) {
    return Rgb{static_cast<uint8_t>(color.r * level / 255),
               static_cast<uint8_t>(color.g * level / 255),
               static_cast<uint8_t>(color.b * level / 255)};
}

// Level 0 is `from`, 255 is `to`.
Rgb blended(Rgb from, Rgb to, uint8_t level) {
    return Rgb{static_cast<uint8_t>(from.r + (to.r - from.r) * level / 255),
               static_cast<uint8_t>(from.g + (to.g - from.g) * level / 255),
               static_cast<uint8_t>(from.b + (to.b - from.b) * level / 255)};
}

}  // namespace

void StatusLeds::update(uint32_t clock, bool running) {
    for (const Zone& zone : m_zones) {
        for (uint16_t i = 0; i < zone.leds.count; i++) {
            Rgb color = colorAt(zone, i, clock, running);
            m_strip.setPixelColor(zone.leds.first + i, color.r, color.g, color.b);
        }
    }
    m_strip.show();
}

Rgb StatusLeds::colorAt(const Zone& zone, size_t position, uint32_t clock, bool running) const {
    // The belt works for the whole run, so it stays idle.
    Station::Phase phase =
        zone.station->continuous() ? Station::Phase::Idle : zone.station->phase();
    switch (phase) {
        case Station::Phase::Idle:
            return running ? m_config.runningIdle : zone.idle;
        case Station::Phase::Arriving:
            return inBand(position, zone.leds.count, clock) ? m_config.arriving : kOff;
        case Station::Phase::Working:
            if (m_config.work == WorkLook::kProgress) {
                return progressAt(zone, position, clock, zone.station->workProgress(clock));
            }
            return dimmed(zone.pulse.color, pulseLevel(clock, zone.pulse.periodMs));
        case Station::Phase::Done:
            return m_config.work == WorkLook::kProgress ? m_config.fillTo : zone.pulse.color;
        case Station::Phase::Clearing:
            return inBand(position, zone.leds.count, clock) ? m_config.clearing : kOff;
    }
    return kOff;
}

// The bar reaches the first LED first, then each LED after it in turn.
Rgb StatusLeds::progressAt(const Zone& zone, size_t position, uint32_t clock,
                           uint8_t progress) const {
    uint32_t filled = static_cast<uint32_t>(progress) * zone.leds.count;
    uint32_t start = static_cast<uint32_t>(position) * 255;
    uint32_t reached = filled > start ? filled - start : 0;
    uint8_t level = reached > 255 ? 255 : static_cast<uint8_t>(reached);
    return dimmed(blended(m_config.fillFrom, m_config.fillTo, level),
                  pulseLevel(clock, zone.pulse.periodMs));
}

bool StatusLeds::inBand(size_t position, size_t count, uint32_t clock) const {
    size_t step = clock / m_config.bandStepMs;
    if (m_config.band == BandMotion::kWrap) {
        size_t start = step % count;
        // Distance from the band's first LED, wrapping past the end of the zone.
        return (position + count - start) % count < m_config.bandWidth;
    }
    size_t travel = count > m_config.bandWidth ? count - m_config.bandWidth : 0;
    size_t start = step % (travel + 1);
    return position >= start && position < start + m_config.bandWidth;
}

void StatusLeds::selfTest() {
    int total = 0;
    for (const Zone& zone : m_zones) {
        for (uint16_t i = 0; i < zone.leds.count; i++) {
            m_strip.setPixelColor(zone.leds.first + i, 255 - total, total, 0);
            total++;
        }
    }
    logLine("set %d LEDs", total);
    m_strip.show();
}
