#ifndef GCPUSHER_H
#define GCPUSHER_H

#include <P1AM.h>

#include "machine/Station.h"

// DRAFT. Pick-and-place a graham cracker onto the tray with three pneumatic
// solenoids, sequenced purely on time:
//
//   grab -> lift -> translate -> lower -> release -> return home
//
// Return home is not one of the named moves but is required, or the arm stays
// over the tray and the next cycle has nowhere to grab from.
class GcPusher : public Station {
   public:
    static const size_t kMoveCount = 6;

    struct Config {
        channelLabel capture;    // Tray stop, kAbsent if the tray is not held
        channelLabel gripper;    // Energize to grip
        channelLabel lift;       // Energize to raise
        channelLabel translate;  // Energize to swing over the tray

        // Hold time per move, in order: grab, lift, translate, lower, release,
        // return. Their sum is the station's work duration.
        uint32_t moveMs[kMoveCount];

        uint32_t transitMs;  // Upstream release -> tray arrives here
        uint32_t clearMs;    // Release -> tray fully past this station
    };

    GcPusher(std::string name, P1AM& p1, Config config);

    void selfTest() override;

   protected:
    void onActivate() override;
    void onArrive() override;
    void onWork(uint32_t elapsedMs) override;
    void onComplete() override;
    void onRelease() override;
    void onEStop() override;

   private:
    static Timing timingFor(const Config& config);
    void applyMove(size_t index);
    void parkArm();

    P1AM& m_p1;
    Config m_config;
    size_t m_move = kMoveCount;  // Out of range, so the first move always writes
};

#endif
