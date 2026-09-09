#ifndef LINEARDISPENSER_H
#define LINEARDISPENSER_H

#include <P1AM.h>

#include "machine/Station.h"

// Dispenses by driving a linear actuator out and back: a DIO energizes a relay
// that is normally retracted; energized, the relay extends the actuator until
// it hits its own internal limit switch, mechanically, with no feedback wired
// back to us. extendMs must be long enough to reach that limit. The work
// phase is extend, dwell at full extension, retract, so its duration always
// matches the stroke it actually commands.
class LinearDispenser : public Station {
   public:
    struct Config {
        channelLabel capture;  // Tray stop
        channelLabel extend;   // Energize to drive the actuator out
        uint32_t extendMs;     // Travel out
        uint32_t dwellMs;      // Hold at full extension
        uint32_t retractMs;    // Travel back
        uint32_t transitMs;    // Upstream release -> tray arrives here
        uint32_t clearMs;      // Release -> tray fully past this station
    };

    LinearDispenser(std::string name, P1AM& p1, Config config);

    void selfTest() override;

   protected:
    void onActivate() override;
    void onArrive() override;
    bool onWork(uint32_t elapsedMs) override;
    void onComplete() override;
    void onRelease() override;
    void onEStop() override;

   private:
    static Timing timingFor(const Config& config);
    void setExtended(bool extended);

    P1AM& m_p1;
    Config m_config;
    bool m_extended = false;
};

#endif
