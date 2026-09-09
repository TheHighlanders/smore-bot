#include "stations/Oven.h"

#include <Arduino.h>
#include <math.h>

#include "Channel.h"
#include "Log.h"

// A reading outside this band means a failed probe or a failed SPI read, both
// of which return plausible-looking values (NaN on burnout, 0.0 on timeout).
static const float kMinPlausibleF = 32.0f;
static const float kMaxPlausibleF = 500.0f;

Oven::Oven(std::string name, P1AM& p1, Config config, Timing timing)
    : Station(name, timing),
      m_p1(p1),
      m_config(config),
      m_forceAtTemp(name + "temp", PERSISTENT) {}

void Oven::poll(bool machineRunning) {
    if (!machineRunning) {
        writeChannel(m_p1, 0, m_config.heater);
        return;
    }

    if (!fitted(m_config.thermistor)) {
        setAtTemp(true);  // No probe fitted: run the oven open-loop.
        return;
    }

    m_temperature = m_p1.readTemperature(m_config.thermistor);

    if (!isfinite(m_temperature) || m_temperature < kMinPlausibleF ||
        m_temperature > kMaxPlausibleF) {
        writeChannel(m_p1, 0, m_config.heater);
        setAtTemp(false);
        logError("%s: implausible probe reading, heater off", name().c_str());
    } else if (m_temperature < m_config.setpointF - m_config.deadbandF) {
        writeChannel(m_p1, 1, m_config.heater);
        setAtTemp(false);
    } else {
        // Anywhere at or above the band is hot enough to cook, so an overshoot
        // cannot strand the line waiting for the oven to report ready.
        if (m_temperature > m_config.setpointF + m_config.deadbandF) {
            writeChannel(m_p1, 0, m_config.heater);
        }
        setAtTemp(true);
    }

    if (m_forceAtTemp.read()) {
        setAtTemp(true);
    }
}

void Oven::onActivate() { writeChannel(m_p1, 1, m_config.hold); }

void Oven::onRelease() { writeChannel(m_p1, 0, m_config.hold); }

void Oven::onEStop() {
    writeChannel(m_p1, 0, m_config.heater);
    writeChannel(m_p1, 0, m_config.hold);
}

std::string Oven::detail() const {
    if (!fitted(m_config.thermistor)) {
        return "no probe";
    }
    return std::to_string(static_cast<int>(m_temperature)) + "F";
}

void Oven::setAtTemp(bool value) {
    if (value == m_atTemp) {
        return;
    }
    m_atTemp = value;
    logUpdate("%s: %s", name().c_str(), value ? "at temperature" : "heating");
}

void Oven::selfTest() {
    logUpdate("%s: tray hold solenoid", name().c_str());
    writeChannel(m_p1, 1, m_config.hold);
    delay(kPulseMs);
    writeChannel(m_p1, 0, m_config.hold);

    logUpdate("%s: heater relay", name().c_str());
    writeChannel(m_p1, 1, m_config.heater);
    delay(kPulseMs);
    writeChannel(m_p1, 0, m_config.heater);

    if (!fitted(m_config.thermistor)) {
        logInfo("\tthermistor: not fitted");
    } else {
        logInfo("\tthermistor: %d F", (int)m_p1.readTemperature(m_config.thermistor));
    }
}
