#include "stations/Oven.h"

#include <Arduino.h>

#include "Log.h"

Oven::Oven(std::string name, P1AM& p1, Config config)
    : Station(name, timingFor(config)), m_p1(p1), m_config(config) {}

Station::Timing Oven::timingFor(const Config& config) {
    return Timing{config.transitMs, config.cookMs, config.clearMs};
}

void Oven::poll(bool machineRunning) {
    if (!m_config.enabled) {
        return;  // Not under test: no hardware touched.
    }
    m_p1.writeDiscrete(machineRunning ? 1 : 0, m_config.heater);
    m_temperature = m_p1.readTemperature(m_config.thermistor);  // Display only
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

std::string Oven::detail() const {
    if (!m_config.enabled) {
        return "disabled";
    }
    return std::to_string(static_cast<int>(m_temperature)) + "F";
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
