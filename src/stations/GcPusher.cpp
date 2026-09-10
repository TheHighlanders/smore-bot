#include "stations/GcPusher.h"

#include <Arduino.h>

#include "Log.h"

namespace {

struct Move {
    bool lifterUp;
    bool clawClosed;
    bool pusherOut;
    uint32_t ms;
};

// Positions and hold times for one full cycle. States and durations are
// bundled per move rather than split apart, so the sequence reads as one
// list end to end - see the header for the walk-through.
const Move kMoves[] = {
    {true, false, true, 5000},
    {false, false, true, 2000},
    {false, true, true, 2000},
    {true, true, false, 5000},
    {false, true, false, 2000},
    {false, false, false, 1000},
    {true, false, false, 2000},
};
const size_t kMoveCount = sizeof(kMoves) / sizeof(kMoves[0]);

}  // namespace

GcPusher::GcPusher(std::string name, P1AM& p1, Config config)
    : Station(name, timingFor(config)), m_p1(p1), m_config(config) {}

Station::Timing GcPusher::timingFor(const Config& config) {
    uint32_t total = 0;
    for (size_t i = 0; i < kMoveCount; i++) {
        total += kMoves[i].ms;
    }
    return Timing{config.transitMs, total, config.clearMs};
}

void GcPusher::onActivate() { m_p1.writeDiscrete(1, m_config.capture); }

void GcPusher::onArrive() {
    m_move = kMoveCount;  // Replay the sequence from the start
    applyMove(0);
}

bool GcPusher::onWork(uint32_t elapsedMs) {
    uint32_t boundary = 0;
    for (size_t i = 0; i < kMoveCount; i++) {
        boundary += kMoves[i].ms;
        if (elapsedMs < boundary) {
            applyMove(i);
            return false;
        }
    }
    applyMove(kMoveCount - 1);
    return false;
}

void GcPusher::onRelease() { m_p1.writeDiscrete(0, m_config.capture); }

void GcPusher::onEStop() {
    m_p1.writeDiscrete(0, m_config.capture);
    m_p1.writeDiscrete(0, m_config.lifter);
    m_p1.writeDiscrete(0, m_config.claw);
    m_p1.writeDiscrete(0, m_config.pusher);
    m_move = kMoveCount;
}

void GcPusher::applyMove(size_t index) {
    if (index == m_move) {
        return;
    }
    m_move = index;
    const Move& move = kMoves[index];
    m_p1.writeDiscrete(move.lifterUp, m_config.lifter);
    m_p1.writeDiscrete(move.clawClosed, m_config.claw);
    m_p1.writeDiscrete(move.pusherOut, m_config.pusher);
    logLine("%s: lifter %s, claw %s, pusher %s", name().c_str(), move.lifterUp ? "up" : "down",
            move.clawClosed ? "closed" : "open", move.pusherOut ? "out" : "in");
}

void GcPusher::selfTest() {
    logLine("%s: tray stop", name().c_str());
    m_p1.writeDiscrete(1, m_config.capture);
    delay(kPulseMs);
    m_p1.writeDiscrete(0, m_config.capture);

    logLine("%s: full sequence", name().c_str());
    for (size_t i = 0; i < kMoveCount; i++) {
        applyMove(i);
        delay(kMoves[i].ms);
    }
}
