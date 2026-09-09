#include "stations/GcPusher.h"

#include <Arduino.h>

#include "Log.h"

namespace {

struct Move {
    const char* name;
    bool gripper;
    bool lift;
    bool translate;
};

// Solenoid states through the cycle. Durations come from Config::moveMs.
const Move kMoves[GcPusher::kMoveCount] = {
    {"grab", true, false, false},       //
    {"lift", true, true, false},        //
    {"translate", true, true, true},    //
    {"lower", true, false, true},       //
    {"release", false, false, true},    //
    {"return", false, false, false},    //
};

}  // namespace

GcPusher::GcPusher(std::string name, P1AM& p1, Config config)
    : Station(name, timingFor(config)), m_p1(p1), m_config(config) {}

Station::Timing GcPusher::timingFor(const Config& config) {
    uint32_t total = 0;
    for (size_t i = 0; i < kMoveCount; i++) {
        total += config.moveMs[i];
    }
    return Timing{config.transitMs, total, config.clearMs};
}

void GcPusher::onActivate() { m_p1.writeDiscrete(1, m_config.capture); }

void GcPusher::onArrive() {
    m_move = kMoveCount;  // Replay the sequence from the start
    applyMove(0);
}

void GcPusher::onWork(uint32_t elapsedMs) {
    uint32_t boundary = 0;
    for (size_t i = 0; i < kMoveCount; i++) {
        boundary += m_config.moveMs[i];
        if (elapsedMs < boundary) {
            applyMove(i);
            return;
        }
    }
    applyMove(kMoveCount - 1);
}

void GcPusher::onComplete() { parkArm(); }

void GcPusher::onRelease() { m_p1.writeDiscrete(0, m_config.capture); }

void GcPusher::onEStop() {
    m_p1.writeDiscrete(0, m_config.capture);
    parkArm();
}

void GcPusher::applyMove(size_t index) {
    if (index == m_move) {
        return;
    }
    m_move = index;
    m_p1.writeDiscrete(kMoves[index].gripper, m_config.gripper);
    m_p1.writeDiscrete(kMoves[index].lift, m_config.lift);
    m_p1.writeDiscrete(kMoves[index].translate, m_config.translate);
    logInfo("%s: %s", name().c_str(), kMoves[index].name);
}

void GcPusher::parkArm() {
    m_move = kMoveCount;
    m_p1.writeDiscrete(0, m_config.gripper);
    m_p1.writeDiscrete(0, m_config.lift);
    m_p1.writeDiscrete(0, m_config.translate);
}

void GcPusher::selfTest() {
    logUpdate("%s: tray stop", name().c_str());
    m_p1.writeDiscrete(1, m_config.capture);
    delay(kPulseMs);
    m_p1.writeDiscrete(0, m_config.capture);

    // Step the arm through the real sequence rather than pulsing solenoids
    // individually, so the moves are checked in an order the rig can survive.
    logUpdate("%s: pick and place sequence", name().c_str());
    for (size_t i = 0; i < kMoveCount; i++) {
        applyMove(i);
        delay(m_config.moveMs[i]);
    }
    parkArm();
}
