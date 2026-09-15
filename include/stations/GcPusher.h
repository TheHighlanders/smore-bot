#ifndef GCPUSHER_H
#define GCPUSHER_H

#include <P1AM.h>

#include <stddef.h>

#include "machine/Station.h"

// Places a graham cracker by stepping the lifter, claw and pusher through
// Config::moves. Each move sets all three outputs and holds them for its own
// duration; the station's work duration is the sum of those holds.
class GcPusher : public Station {
   public:
    struct Move {
        bool lifterUp;
        bool clawClosed;
        bool pusherOut;
        uint32_t ms;  // Hold time
    };

    struct Config {
        channelLabel capture;  // Tray stop
        channelLabel lifter;   // Energize to lower; rests up
        channelLabel claw;     // Energize to close
        channelLabel pusher;   // Energize to push out
        const Move* moves;     // One full cycle, in order
        size_t moveCount;
        uint32_t transitMs;  // Upstream release -> tray arrives here
        uint32_t clearMs;    // Release -> tray fully past this station
    };

    GcPusher(std::string name, P1AM& p1, Config config);

    void selfTest() override;

   protected:
    void onActivate() override;
    void onArrive() override;
    bool onWork(uint32_t elapsedMs) override;
    void onRelease() override;
    void onReset() override;

   private:
    static Timing timingFor(const Config& config);
    void applyMove(size_t index);

    P1AM& m_p1;
    Config m_config;
    size_t m_move;  // Index of the applied move; moveCount before the first
};

#endif
