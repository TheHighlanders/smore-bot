#include "stations/HeatedStop.h"

#include <Arduino.h>

#include "Log.h"

HeatedStop::HeatedStop(std::string name, P1AM& p1, Config config)
    : Station(name, config.timing), m_p1(p1), m_config(config) {}

void HeatedStop::onActivate() { setOn(true); }

void HeatedStop::onRelease() { setOn(false); }

void HeatedStop::onReset() { setOn(false); }

void HeatedStop::setOn(bool on) {
    if (on == m_on) {
        return;
    }
    m_on = on;
    m_p1.writeDiscrete(on ? 1 : 0, m_config.output);
    logLine("%s: %s", name().c_str(), on ? "holding and heating" : "released");
}

void HeatedStop::selfTest() {
    logLine("%s: relay pulse", name().c_str());
    setOn(true);
    delay(kPulseMs);
    setOn(false);
}
