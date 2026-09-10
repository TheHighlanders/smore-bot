#ifndef GCPUSHER_H
#define GCPUSHER_H

#include <P1AM.h>

#include "machine/Station.h"

// Places a graham cracker by stepping a lifter, claw, and pusher through a
// fixed list of positions, each held for its own duration. Every move names
// the state of all three outputs together with how long to hold it, so
// holding one output steady across several moves is just repeating its value
// in the table rather than special-cased logic - see kMoves in the .cpp.
class GcPusher : public Station {
   public:
    struct Config {
        channelLabel capture;  // Tray stop
        channelLabel lifter;   // Energize to raise
        channelLabel claw;     // Energize to close
        channelLabel pusher;   // Energize to push out

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
    void onEStop() override;

   private:
    static Timing timingFor(const Config& config);
    void applyMove(size_t index);

    P1AM& m_p1;
    Config m_config;
    size_t m_move;  // Set in the constructor; see kMoveCount in the .cpp
};

#endif
