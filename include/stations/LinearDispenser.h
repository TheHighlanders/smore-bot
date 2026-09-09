#ifndef LINEARDISPENSER_H
#define LINEARDISPENSER_H

#include <P1AM.h>

#include "machine/Station.h"

// DRAFT. Dispenses by driving a linear actuator out and back on its own timer:
// extend, dwell at full extension, retract. The work phase lasts exactly as
// long as that stroke, so the two cannot be configured out of step.
class LinearDispenser : public Station {
   public:
    struct Config {
        channelLabel capture;  // Tray stop, kAbsent if the tray is not held
        channelLabel extend;   // Energize to drive the actuator out
        uint32_t extendMs;     // Travel out
        uint32_t dwellMs;      // Hold at full extension
        uint32_t retractMs;    // Travel back
    };

    LinearDispenser(std::string name, P1AM& p1, Config config, uint32_t transitMs,
                    uint32_t clearMs);

    void selfTest() override;

   protected:
    void onActivate() override;
    void onArrive() override;
    void onWork(uint32_t elapsedMs) override;
    void onComplete() override;
    void onRelease() override;
    void onEStop() override;

   private:
    static Timing timingFor(const Config& config, uint32_t transitMs, uint32_t clearMs);
    void setExtended(bool extended);

    P1AM& m_p1;
    Config m_config;
    bool m_extended = false;
};

#endif
