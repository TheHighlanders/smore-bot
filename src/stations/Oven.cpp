#include "stations/Oven.h"

#include <Arduino.h>

#include "Log.h"

Oven::Oven(std::string name, P1AM& p1, Config config, ReadyCheck readyCheck)
    : Station(name, timingFor(config)), m_p1(p1), m_config(config), m_readyCheck(readyCheck) {}

Station::Timing Oven::timingFor(const Config& config) {
    return Timing{config.transitMs, config.cookMs, config.clearMs};
}

// Runs in every phase so the status line always shows a current reading.
void Oven::poll() {
    if (m_config.enabled) {
        m_temperature = m_p1.readTemperature(m_config.thermistor);
    }
}

bool Oven::ready() const { return m_readyCheck(millis()); }

void Oven::onActivate() {
    setHeating(true);
    setHeld(true);
}

void Oven::onArrive() { m_working = true; }

bool Oven::onWork(uint32_t /*elapsedMs*/) {
    if (m_cancelRequested) {
        m_cancelRequested = false;
        return true;
    }
    return false;
}

void Oven::onComplete() {
    setHeating(false);
    m_working = false;
}

void Oven::onRelease() { setHeld(false); }

void Oven::onReset() {
    setHeating(false);
    setHeld(true);
    m_working = false;
    m_cancelRequested = false;
}

bool Oven::cancelCook() {
    if (!m_working) {
        return false;
    }
    m_cancelRequested = true;
    return true;
}

void Oven::setHeating(bool on) {
    if (m_config.enabled) {
        m_p1.writeDiscrete(on, m_config.heater);
    }
}

void Oven::setHeld(bool held) {
    if (m_config.enabled) {
        m_p1.writeDiscrete(held ? 0 : 1, m_config.hold);  // Energized releases; rests captured
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
