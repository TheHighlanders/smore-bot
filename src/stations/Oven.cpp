#include "stations/Oven.h"

#include <Arduino.h>
#include <math.h>

#include "Log.h"

// A reading outside this band means a failed probe or a failed SPI read, both
// of which return plausible-looking values (NaN on burnout, 0.0 on timeout).
static const float kMinPlausibleF = 32.0f;
static const float kMaxPlausibleF = 500.0f;

Oven::Oven(std::string name, P1AM& p1, Config config)
    : Station(name, timingFor(config)),
      m_p1(p1),
      m_config(config),
      m_forceAtTemp(name + "temp", PERSISTENT) {}

Station::Timing Oven::timingFor(const Config& config) {
    return Timing{config.transitMs, config.cookMs, config.clearMs};
}

void Oven::poll(bool machineRunning) {
    if (!m_config.enabled) {
        return;  // Not under test: no hardware touched, always ready.
    }

    if (!machineRunning) {
        m_p1.writeDiscrete(0, m_config.heater);
        return;
    }

    m_temperature = m_p1.readTemperature(m_config.thermistor);

    if (!isfinite(m_temperature) || m_temperature < kMinPlausibleF ||
        m_temperature > kMaxPlausibleF) {
        m_p1.writeDiscrete(0, m_config.heater);
        setAtTemp(false);
        logLine("%s: implausible probe reading, heater off", name().c_str());
    } else if (m_temperature < m_config.setpointF - m_config.deadbandF) {
        m_p1.writeDiscrete(1, m_config.heater);
        setAtTemp(false);
    } else {
        // Anywhere at or above the band is hot enough to cook, so an overshoot
        // cannot strand the line waiting for the oven to report ready.
        if (m_temperature > m_config.setpointF + m_config.deadbandF) {
            m_p1.writeDiscrete(0, m_config.heater);
        }
        setAtTemp(true);
    }

    if (m_forceAtTemp.read()) {
        setAtTemp(true);
    }
}

void Oven::onActivate() {
    if (m_config.enabled) {
        m_p1.writeDiscrete(1, m_config.hold);
    }
}

void Oven::onRelease() {
    if (m_config.enabled) {
        m_p1.writeDiscrete(0, m_config.hold);
    }
}

void Oven::onEStop() {
    if (m_config.enabled) {
        m_p1.writeDiscrete(0, m_config.heater);
        m_p1.writeDiscrete(0, m_config.hold);
    }
}

std::string Oven::detail() const {
    if (!m_config.enabled) {
        return "disabled";
    }
    return std::to_string(static_cast<int>(m_temperature)) + "F";
}

void Oven::setAtTemp(bool value) {
    if (value == m_atTemp) {
        return;
    }
    m_atTemp = value;
    logLine("%s: %s", name().c_str(), value ? "at temperature" : "heating");
}

void Oven::selfTest() {
    if (!m_config.enabled) {
        logLine("%s: disabled, skipping", name().c_str());
        return;
    }

    logLine("%s: tray hold solenoid", name().c_str());
    m_p1.writeDiscrete(1, m_config.hold);
    delay(kPulseMs);
    m_p1.writeDiscrete(0, m_config.hold);

    logLine("%s: heater relay", name().c_str());
    m_p1.writeDiscrete(1, m_config.heater);
    delay(kPulseMs);
    m_p1.writeDiscrete(0, m_config.heater);

    logLine("\tthermistor: %d F", (int)m_p1.readTemperature(m_config.thermistor));
}
