#ifndef GCPUSHER_H
#define GCPUSHER_H

#include <P1AM.h>

#include "machine/Station.h"

// Places a graham cracker with a linear actuator plus a two-axis pick arm
// (a gripper and a single up/down lift, no lateral travel):
//
//   push (linear actuator ejects a cracker to the pickup point)
//   lower (arm descends onto it)
//   grab (grip closes)
//   raise (lifts clear)
//   release (grip opens)
//
// push's own retract is not on this timeline: it is commanded off the moment
// push finishes and travels back on its own time, in parallel with
// lower/grab/raise/release.
//
// ASSUMPTION, pending a bring-up test: grab/lift polarity (energized = close
// / lower, de-energized = open / raise, both spring-return) and the release
// step, which completes the place but was not spelled out in the brief.
class GcPusher : public Station {
   public:
    struct Config {
        channelLabel capture;  // Tray stop
        channelLabel push;     // Linear actuator: ejects a cracker to the pickup point
        channelLabel grab;     // Gripper: energize to close
        channelLabel lift;     // Energize to lower, de-energize to raise

        uint32_t pushMs;     // Push extends this long, until its own limit switch
        uint32_t lowerMs;    // Time to lower once pushed into place
        uint32_t grabMs;     // Time to grip once lowered
        uint32_t raiseMs;    // Time to raise clear once gripped
        uint32_t releaseMs;  // Time held open after releasing, before considered placed

        uint32_t transitMs;  // Upstream release -> tray arrives here
        uint32_t clearMs;    // Release -> tray fully past this station
    };

    GcPusher(std::string name, P1AM& p1, Config config);

    void selfTest() override;

   protected:
    bool onWork(uint32_t elapsedMs) override;
    void onComplete() override;
    void onActivate() override;
    void onRelease() override;
    void onEStop() override;

   private:
    static Timing timingFor(const Config& config);
    void setPush(bool extend);
    void setLift(bool lower);
    void setGrab(bool close);
    void park();

    P1AM& m_p1;
    Config m_config;
    bool m_pushOn = false;
    bool m_liftDown = false;
    bool m_gripClosed = false;
};

#endif
