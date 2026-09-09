#include "stations/MotorDispenser.h"

#include <Arduino.h>

#include "Log.h"

MotorDispenser::MotorDispenser(std::string name, P1AM& p1, Config config)
    : Station(name, timingFor(config)), m_p1(p1), m_config(config) {}

Station::Timing MotorDispenser::timingFor(const Config& config) {
    return Timing{config.transitMs, config.timeoutMs, config.clearMs};
}

void MotorDispenser::onActivate() { m_p1.writeDiscrete(1, m_config.capture); }

void MotorDispenser::onArrive() { setRunning(true); }

bool MotorDispenser::onWork(uint32_t) {
    if (!m_p1.readDiscrete(m_config.exitSensor)) {
        return false;
    }
    setRunning(false);
    return true;
}

void MotorDispenser::onComplete() {
    if (m_running) {
        // Reached here via the timeoutMs bound, not the sensor.
        logLine("%s: exit sensor never triggered, stopping on timeout", name().c_str());
        setRunning(false);
    }
}

void MotorDispenser::onRelease() { m_p1.writeDiscrete(0, m_config.capture); }

void MotorDispenser::onEStop() {
    m_p1.writeDiscrete(0, m_config.capture);
    setRunning(false);
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

    logLine("%s: motor until exit sensor (timeout %lums)", name().c_str(),
            (unsigned long)m_config.timeoutMs);
    setRunning(true);
    uint32_t start = millis();
    while (millis() - start < m_config.timeoutMs && !m_p1.readDiscrete(m_config.exitSensor)) {
        delay(20);
    }
    setRunning(false);
    logLine("%s: exit sensor %s", name().c_str(),
            m_p1.readDiscrete(m_config.exitSensor) ? "triggered" : "timed out");
}
