#include "stations/LinearDispenser.h"

#include <Arduino.h>

#include "Log.h"

LinearDispenser::LinearDispenser(std::string name, P1AM& p1, Config config)
    : Station(name, timingFor(config)), m_p1(p1), m_config(config) {}

Station::Timing LinearDispenser::timingFor(const Config& config) {
    uint32_t stroke = config.extendMs + config.dwellMs + config.retractMs;
    return Timing{config.transitMs, stroke, config.clearMs};
}

void LinearDispenser::onActivate() { m_p1.writeDiscrete(1, m_config.capture); }

void LinearDispenser::onArrive() { setExtended(true); }

void LinearDispenser::onWork(uint32_t elapsedMs) {
    setExtended(elapsedMs < m_config.extendMs + m_config.dwellMs);
}

void LinearDispenser::onComplete() { setExtended(false); }

void LinearDispenser::onRelease() { m_p1.writeDiscrete(0, m_config.capture); }

void LinearDispenser::onEStop() {
    m_p1.writeDiscrete(0, m_config.capture);
    setExtended(false);
}

void LinearDispenser::setExtended(bool extended) {
    if (extended == m_extended) {
        return;
    }
    m_extended = extended;
    m_p1.writeDiscrete(extended ? 1 : 0, m_config.extend);
    logLine("%s: actuator %s", name().c_str(), extended ? "extending" : "retracting");
}

void LinearDispenser::selfTest() {
    logLine("%s: tray stop", name().c_str());
    m_p1.writeDiscrete(1, m_config.capture);
    delay(kPulseMs);
    m_p1.writeDiscrete(0, m_config.capture);

    logLine("%s: actuator full stroke", name().c_str());
    setExtended(true);
    delay(m_config.extendMs + m_config.dwellMs);
    setExtended(false);
    delay(m_config.retractMs);
}
