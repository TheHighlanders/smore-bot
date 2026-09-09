#ifndef BELT_H
#define BELT_H

#include <P1AM.h>

#include "machine/Station.h"

// Runs whenever the machine runs. Never completes on its own.
class Belt : public Station {
   public:
    Belt(std::string name, P1AM& p1, channelLabel relay)
        : Station(name, Timing{0, kContinuous, 0}), m_p1(p1), m_relay(relay) {}

   protected:
    void onActivate() override { m_p1.writeDiscrete(1, m_relay); }
    void onRelease() override { m_p1.writeDiscrete(0, m_relay); }
    void onEStop() override { m_p1.writeDiscrete(0, m_relay); }

   private:
    P1AM& m_p1;
    channelLabel m_relay;
};

#endif
