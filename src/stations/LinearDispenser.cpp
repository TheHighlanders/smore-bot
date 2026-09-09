#include "stations/LinearDispenser.h"

#include <Arduino.h>

#include "Channel.h"
#include "Log.h"

LinearDispenser::LinearDispenser(std::string name, P1AM& p1, Config config)
    : Station(name, timingFor(config)), m_p1(p1), m_config(config) {}

Station::Timing LinearDispenser::timingFor(const Config& config) {
    uint32_t stroke = config.extendMs + config.dwellMs + config.retractMs;
    return Timing{config.transitMs, stroke, config.clearMs};
}

void LinearDispenser::onActivate() { writeChannel(m_p1, 1, m_config.capture); }

void LinearDispenser::onArrive() { setExtended(true); }

void LinearDispenser::onWork(uint32_t elapsedMs) {
    setExtended(elapsedMs < m_config.extendMs + m_config.dwellMs);
}

void LinearDispenser::onComplete() { setExtended(false); }

void LinearDispenser::onRelease() { writeChannel(m_p1, 0, m_config.capture); }

void LinearDispenser::onEStop() {
    writeChannel(m_p1, 0, m_config.capture);
    setExtended(false);
}

void LinearDispenser::setExtended(bool extended) {
    if (extended == m_extended) {
        return;
    }
    m_extended = extended;
    writeChannel(m_p1, extended ? 1 : 0, m_config.extend);
    logInfo("%s: actuator %s", name().c_str(), extended ? "extending" : "retracting");
}

void LinearDispenser::selfTest() {
    logUpdate("%s: tray stop", name().c_str());
    writeChannel(m_p1, 1, m_config.capture);
    delay(kPulseMs);
    writeChannel(m_p1, 0, m_config.capture);

    logUpdate("%s: actuator full stroke", name().c_str());
    setExtended(true);
    delay(m_config.extendMs + m_config.dwellMs);
    setExtended(false);
    delay(m_config.retractMs);
}
