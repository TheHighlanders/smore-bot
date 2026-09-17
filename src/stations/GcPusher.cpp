#include "stations/GcPusher.h"

#include <Arduino.h>

#include "Log.h"

GcPusher::GcPusher(std::string name, P1AM& p1, Config config)
    : Station(name, timingFor(config)), m_p1(p1), m_config(config), m_move(config.moveCount) {}

Station::Timing GcPusher::timingFor(const Config& config) {
    uint32_t total = 0;
    for (size_t i = 0; i < config.moveCount; i++) {
        total += config.moves[i].ms;
    }
    return Timing{config.transitMs, total, config.clearMs};
}

void GcPusher::onActivate() { setCaptured(true); }

void GcPusher::onArrive() {
    m_move = m_config.moveCount;  // Replay the sequence from the start
    applyMove(0);
}

bool GcPusher::onWork(uint32_t elapsedMs) {
    uint32_t boundary = 0;
    for (size_t i = 0; i < m_config.moveCount; i++) {
        boundary += m_config.moves[i].ms;
        if (elapsedMs < boundary) {
            applyMove(i);
            return false;
        }
    }
    applyMove(m_config.moveCount - 1);
    return false;
}

void GcPusher::onRelease() {
    setCaptured(false);
    m_releasedAt = millis();
}

void GcPusher::onReset() {
    m_p1.writeDiscrete(0, m_config.capture);
    m_p1.writeDiscrete(0, m_config.lifter);
    m_p1.writeDiscrete(0, m_config.claw);
    m_p1.writeDiscrete(0, m_config.pusher);
    m_move = m_config.moveCount;
    m_captured = true;
}

// Re-captures once the tray has cleared, instead of leaving the stop open
// until the next tray activates this station.
void GcPusher::poll() {
    if (!m_captured && millis() - m_releasedAt >= m_config.clearMs) {
        setCaptured(true);
    }
}

void GcPusher::setCaptured(bool captured) {
    if (captured == m_captured) {
        return;
    }
    m_captured = captured;
    m_p1.writeDiscrete(captured ? 0 : 1, m_config.capture);  // Energized releases; rests captured
    logLine("%s: tray stop %s", name().c_str(), captured ? "capturing" : "releasing");
}

void GcPusher::applyMove(size_t index) {
    if (index == m_move) {
        return;
    }
    m_move = index;
    const Move& move = m_config.moves[index];
    m_p1.writeDiscrete(!move.lifterUp, m_config.lifter);  // Energized lowers the lifter
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
    for (size_t i = 0; i < m_config.moveCount; i++) {
        applyMove(i);
        delay(m_config.moves[i].ms);
    }
}
