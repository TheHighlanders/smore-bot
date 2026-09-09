#include "stations/GcPusher.h"

#include <Arduino.h>

#include "Log.h"

GcPusher::GcPusher(std::string name, P1AM& p1, Config config)
    : Station(name, timingFor(config)), m_p1(p1), m_config(config) {}

Station::Timing GcPusher::timingFor(const Config& config) {
    uint32_t workMs =
        config.pushMs + config.lowerMs + config.grabMs + config.raiseMs + config.releaseMs;
    return Timing{config.transitMs, workMs, config.clearMs};
}

void GcPusher::onActivate() { m_p1.writeDiscrete(1, m_config.capture); }

bool GcPusher::onWork(uint32_t elapsedMs) {
    uint32_t lowered = m_config.pushMs;
    uint32_t grabbed = lowered + m_config.lowerMs;
    uint32_t raised = grabbed + m_config.grabMs;
    uint32_t released = raised + m_config.raiseMs;

    setPush(elapsedMs < lowered);
    setLift(elapsedMs >= lowered && elapsedMs < raised);
    setGrab(elapsedMs >= grabbed && elapsedMs < released);
    return false;
}

void GcPusher::onComplete() { park(); }

void GcPusher::onRelease() { m_p1.writeDiscrete(0, m_config.capture); }

void GcPusher::onEStop() {
    m_p1.writeDiscrete(0, m_config.capture);
    park();
}

void GcPusher::setPush(bool extend) {
    if (extend == m_pushOn) {
        return;
    }
    m_pushOn = extend;
    m_p1.writeDiscrete(extend ? 1 : 0, m_config.push);
    logLine("%s: push %s", name().c_str(), extend ? "extending" : "retracting");
}

void GcPusher::setLift(bool lower) {
    if (lower == m_liftDown) {
        return;
    }
    m_liftDown = lower;
    m_p1.writeDiscrete(lower ? 1 : 0, m_config.lift);
    logLine("%s: arm %s", name().c_str(), lower ? "lowering" : "raising");
}

void GcPusher::setGrab(bool close) {
    if (close == m_gripClosed) {
        return;
    }
    m_gripClosed = close;
    m_p1.writeDiscrete(close ? 1 : 0, m_config.grab);
    logLine("%s: gripper %s", name().c_str(), close ? "closing" : "opening");
}

void GcPusher::park() {
    setPush(false);
    setLift(false);
    setGrab(false);
}

void GcPusher::selfTest() {
    logLine("%s: tray stop", name().c_str());
    m_p1.writeDiscrete(1, m_config.capture);
    delay(kPulseMs);
    m_p1.writeDiscrete(0, m_config.capture);

    logLine("%s: push", name().c_str());
    setPush(true);
    delay(m_config.pushMs);
    setPush(false);

    logLine("%s: lower and grab", name().c_str());
    setLift(true);
    delay(m_config.lowerMs);
    setGrab(true);
    delay(m_config.grabMs);

    logLine("%s: raise and release", name().c_str());
    setLift(false);
    delay(m_config.raiseMs);
    setGrab(false);
    delay(m_config.releaseMs);
}
