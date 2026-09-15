#ifndef BELT_H
#define BELT_H

#include <P1AM.h>

#include <Arduino.h>

#include "Log.h"
#include "machine/Station.h"

// Runs while the machine runs.
class Belt : public Station {
   public:
    Belt(std::string name, P1AM& p1, channelLabel relay)
        : Station(name, Timing{0, kContinuous, 0}), m_p1(p1), m_relay(relay) {}

    void selfTest() override {
        logLine("%s: belt relay", name().c_str());
        m_p1.writeDiscrete(1, m_relay);
        delay(kPulseMs);
        m_p1.writeDiscrete(0, m_relay);
    }

   protected:
    void onActivate() override { m_p1.writeDiscrete(1, m_relay); }
    void onReset() override { m_p1.writeDiscrete(0, m_relay); }

   private:
    P1AM& m_p1;
    channelLabel m_relay;
};

#endif
