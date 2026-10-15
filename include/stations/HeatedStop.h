#ifndef HEATEDSTOP_H
#define HEATEDSTOP_H

#include <P1AM.h>

#include "machine/Station.h"

// Stops the tray and heats it with one relay. The relay is on from activation
// until release, so the stop holds the tray while the heater runs.
class HeatedStop : public Station {
   public:
    struct Config {
        channelLabel output;  // Relay: on holds the tray and runs the heater
        Timing timing;
    };

    HeatedStop(std::string name, P1AM& p1, Config config);

    void selfTest() override;

   protected:
    void onActivate() override;
    void onRelease() override;
    void onReset() override;

   private:
    void setOn(bool on);

    P1AM& m_p1;
    Config m_config;
    bool m_on = false;
};

#endif
