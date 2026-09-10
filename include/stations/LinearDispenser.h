#ifndef LINEARDISPENSER_H
#define LINEARDISPENSER_H

#include <P1AM.h>

#include "machine/Station.h"

// Dispenses by driving a linear actuator out and back: a DIO energizes a relay
// that is normally retracted; energized, the relay extends the actuator until
// it hits its own internal limit switch, mechanically, with no feedback wired
// back to us. There is no third, idle state to hold at full extension - the
// relay is always actively driving out or driving in - so a dwell at full
// extension is just more extend time. extendMs must cover both reaching the
// limit switch and however long the product needs to clear on the way out.
class LinearDispenser : public Station {
   public:
    struct Config {
        channelLabel capture;  // Tray stop
        channelLabel extend;   // Energize to drive the actuator out
        uint32_t extendMs;     // Time to stay extended (travel out plus any hold)
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
    void onRelease() override;

   private:
    static Timing timingFor(const Config& config);
    void setExtended(bool extended);

    P1AM& m_p1;
    Config m_config;
    bool m_extended = false;
};

#endif
