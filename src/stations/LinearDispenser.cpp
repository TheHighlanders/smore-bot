#include "stations/LinearDispenser.h"

#include <Arduino.h>

#include "Log.h"

LinearDispenser::LinearDispenser(std::string name, P1AM& p1, Config config)
    : Station(name, timingFor(config)), m_p1(p1), m_config(config) {}

Station::Timing LinearDispenser::timingFor(const Config& config) {
    return Timing{config.transitMs, config.extendMs + config.retractMs, config.clearMs};
}

void LinearDispenser::onActivate() { setCaptured(true); }

void LinearDispenser::onArrive() { setExtended(true); }

bool LinearDispenser::onWork(uint32_t elapsedMs) {
    setExtended(elapsedMs < m_config.extendMs);
    return false;
}

void LinearDispenser::onRelease() {
    setCaptured(false);
    m_releasedAt = millis();
}

// Re-captures once the tray has cleared, instead of leaving the stop open
// until the next tray activates this station.
void LinearDispenser::poll() {
    if (!m_captured && millis() - m_releasedAt >= m_config.clearMs) {
        setCaptured(true);
    }
}

void LinearDispenser::onReset() {
    m_p1.writeDiscrete(0, m_config.capture);
    m_p1.writeDiscrete(0, m_config.extend);
    m_captured = true;
    m_extended = false;
}

void LinearDispenser::setExtended(bool extended) {
    if (extended == m_extended) {
        return;
    }
    m_extended = extended;
    m_p1.writeDiscrete(extended ? 1 : 0, m_config.extend);
    logLine("%s: actuator %s", name().c_str(), extended ? "extending" : "retracting");
}

void LinearDispenser::setCaptured(bool captured) {
    if (captured == m_captured) {
        return;
    }
    m_captured = captured;
    m_p1.writeDiscrete(captured ? 0 : 1, m_config.capture);  // Energized releases; rests captured
    logLine("%s: tray stop %s", name().c_str(), captured ? "capturing" : "releasing");
}

void LinearDispenser::selfTest() {
    logLine("%s: tray stop", name().c_str());
    setCaptured(false);
    delay(kPulseMs);
    setCaptured(true);

    logLine("%s: actuator full stroke", name().c_str());
    setExtended(true);
    delay(m_config.extendMs);
    setExtended(false);
    delay(m_config.retractMs);
}
