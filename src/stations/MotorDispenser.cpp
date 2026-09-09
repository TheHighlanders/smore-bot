#include "stations/MotorDispenser.h"

#include <Arduino.h>

#include "Channel.h"
#include "Log.h"

MotorDispenser::MotorDispenser(std::string name, P1AM& p1, Config config)
    : Station(name, timingFor(config)), m_p1(p1), m_config(config) {}

Station::Timing MotorDispenser::timingFor(const Config& config) {
    return Timing{config.transitMs, config.runMs + config.settleMs, config.clearMs};
}

void MotorDispenser::onActivate() { writeChannel(m_p1, 1, m_config.capture); }

void MotorDispenser::onArrive() { setRunning(true); }

void MotorDispenser::onWork(uint32_t elapsedMs) { setRunning(elapsedMs < m_config.runMs); }

void MotorDispenser::onComplete() { setRunning(false); }

void MotorDispenser::onRelease() { writeChannel(m_p1, 0, m_config.capture); }

void MotorDispenser::onEStop() {
    writeChannel(m_p1, 0, m_config.capture);
    setRunning(false);
}

void MotorDispenser::setRunning(bool running) {
    if (running == m_running) {
        return;
    }
    m_running = running;
    writeChannel(m_p1, running ? 1 : 0, m_config.motor);
    logInfo("%s: motor %s", name().c_str(), running ? "running" : "stopped");
}

void MotorDispenser::selfTest() {
    logUpdate("%s: tray stop", name().c_str());
    writeChannel(m_p1, 1, m_config.capture);
    delay(kPulseMs);
    writeChannel(m_p1, 0, m_config.capture);

    logUpdate("%s: motor for %lums", name().c_str(), (unsigned long)m_config.runMs);
    setRunning(true);
    delay(m_config.runMs);
    setRunning(false);
}
