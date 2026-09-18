#include "stations/MotorDispenser.h"

#include <Arduino.h>

#include "Log.h"

MotorDispenser::MotorDispenser(std::string name, P1AM& p1, Config config)
    : Station(name, timingFor(config)), m_p1(p1), m_config(config) {}

Station::Timing MotorDispenser::timingFor(const Config& config) {
    return Timing{config.transitMs, config.timeoutMs, config.clearMs};
}

void MotorDispenser::onActivate() { setCaptured(true); }

void MotorDispenser::onArrive() {
    m_seenBlocked = false;
    m_clearing = false;
    setRunning(true);
}

bool MotorDispenser::onWork(uint32_t elapsedMs) {
    bool blocked = m_p1.readDiscrete(m_config.exitSensor);

    if (blocked) {
        m_seenBlocked = true;
        m_clearing = false;
        return false;
    }
    if (!m_seenBlocked) {
        return false;  // Nothing has reached the sensor yet
    }
    if (!m_clearing) {
        m_clearing = true;
        m_clearSinceMs = elapsedMs;
    }
    if (elapsedMs - m_clearSinceMs < kDebounceMs) {
        return false;  // Clear, still debouncing
    }

    setRunning(false);
    return true;
}

void MotorDispenser::onComplete() {
    if (m_running) {
        // Still running here means timeoutMs expired.
        logLine("%s: exit sensor never triggered, stopping on timeout", name().c_str());
        setRunning(false);
    }
}

void MotorDispenser::onRelease() {
    setCaptured(false);
    m_releasedAt = millis();
}

void MotorDispenser::onReset() {
    m_p1.writeDiscrete(0, m_config.capture);
    m_p1.writeDiscrete(0, m_config.motor);
    m_captured = true;
    m_running = false;
}

// Re-captures once the tray has cleared, instead of leaving the stop open
// until the next tray activates this station.
void MotorDispenser::poll() {
    if (!m_captured && millis() - m_releasedAt >= m_config.clearMs) {
        setCaptured(true);
    }
}

void MotorDispenser::setCaptured(bool captured) {
    if (captured == m_captured) {
        return;
    }
    m_captured = captured;
    m_p1.writeDiscrete(captured ? 0 : 1, m_config.capture);  // Energized releases; rests captured
    logLine("%s: tray stop %s", name().c_str(), captured ? "capturing" : "releasing");
}

void MotorDispenser::setRunning(bool running) {
    if (running == m_running) {
        return;
    }
    m_running = running;
    m_p1.writeDiscrete(running ? 1 : 0, m_config.motor);
    logLine("%s: motor %s", name().c_str(), running ? "running" : "stopped");
}

void MotorDispenser::selfTest() {
    logLine("%s: tray stop", name().c_str());
    m_p1.writeDiscrete(1, m_config.capture);
    delay(kPulseMs);
    m_p1.writeDiscrete(0, m_config.capture);

    logLine("%s: motor until exit sensor clears (timeout %lums)", name().c_str(),
            (unsigned long)m_config.timeoutMs);
    setRunning(true);

    uint32_t start = millis();
    bool seenBlocked = false;
    while (millis() - start < m_config.timeoutMs) {
        if (m_p1.readDiscrete(m_config.exitSensor)) {
            seenBlocked = true;
        } else if (seenBlocked) {
            delay(kDebounceMs);
            if (!m_p1.readDiscrete(m_config.exitSensor)) {
                break;
            }
        }
        delay(20);
    }
    setRunning(false);
    logLine("%s: exit sensor %s", name().c_str(), seenBlocked ? "cleared" : "never triggered");
}
