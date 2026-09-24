#include "leds/StatusLeds.h"

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

}  // namespace

void StatusLeds::update(uint32_t clock) {
    for (const Zone& zone : m_zones) {
        for (uint16_t i = 0; i < zone.leds.count; i++) {
            m_strip.set(zone.leds.first + i, colorAt(zone, i, clock));
        }
    }
    m_strip.show();
}

Rgb StatusLeds::colorAt(const Zone& zone, size_t position, uint32_t clock) const {
    switch (zone.station->phase()) {
        case Station::Phase::Idle:
            return m_config.idle;
        case Station::Phase::Arriving:
            return inBand(position, zone.leds.count, clock) ? m_config.arriving : kOff;
        case Station::Phase::Working:
            return dimmed(zone.pulse.color, pulseLevel(clock, zone.pulse.periodMs));
        case Station::Phase::Done:
            return zone.pulse.color;
        case Station::Phase::Clearing:
            return inBand(position, zone.leds.count, clock) ? m_config.clearing : kOff;
    }
    return kOff;
}

bool StatusLeds::inBand(size_t position, size_t count, uint32_t clock) const {
    size_t travel = count > m_config.bandWidth ? count - m_config.bandWidth : 0;
    size_t start = (clock / m_config.bandStepMs) % (travel + 1);
    return position >= start && position < start + m_config.bandWidth;
}
