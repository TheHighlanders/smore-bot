#include "stations/MotorDispenser.h"

#include <Arduino.h>

#include "Log.h"

MotorDispenser::MotorDispenser(std::string name, P1AM& p1, Config config)
    : Station(name, timingFor(config)), m_p1(p1), m_config(config) {}

Station::Timing MotorDispenser::timingFor(const Config& config) {
    return Timing{config.transitMs, config.runMs + config.settleMs, config.clearMs};
}

void MotorDispenser::onActivate() { m_p1.writeDiscrete(1, m_config.capture); }

void MotorDispenser::onArrive() { setRunning(true); }

void MotorDispenser::onWork(uint32_t elapsedMs) { setRunning(elapsedMs < m_config.runMs); }

void MotorDispenser::onComplete() { setRunning(false); }

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

    logLine("%s: motor for %lums", name().c_str(), (unsigned long)m_config.runMs);
    setRunning(true);
    delay(m_config.runMs);
    setRunning(false);
}
