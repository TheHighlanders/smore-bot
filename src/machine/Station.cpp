#include "machine/Station.h"

#include "Log.h"

// Warn once when a finished station waits this long for the next one. Set above
// the longest station occupancy on the line.
static const uint32_t kStallWarnMs = 120000;

void Station::update(uint32_t clock) {
    poll();

    switch (m_phase) {
        case Phase::Arriving:
            if (elapsed(clock) >= m_timing.transitMs) {
                enter(Phase::Working, clock);
                onArrive();
            }
            break;

        case Phase::Working: {
            bool workDone = onWork(elapsed(clock));
            if (m_timing.workMs != kContinuous &&
                (workDone || elapsed(clock) >= m_timing.workMs)) {
                onComplete();
                enter(Phase::Done, clock);
                logLine("%s: work complete", m_name.c_str());
            }
            break;
        }

        case Phase::Done:
            if (!m_stallReported && elapsed(clock) >= kStallWarnMs) {
                m_stallReported = true;
                logLine("%s: blocked %lus waiting on the next station", m_name.c_str(),
                        (unsigned long)(elapsed(clock) / 1000));
            }
            break;

        case Phase::Clearing:
            if (elapsed(clock) >= m_timing.clearMs) {
                enter(Phase::Idle, clock);
            }
            break;

        case Phase::Idle:
            break;
    }
}

uint32_t Station::remainingMs(uint32_t clock) const {
    uint32_t total = 0;
    switch (m_phase) {
        case Phase::Idle:
            return 0;
        case Phase::Done:
            return kContinuous;  // No bound: waiting on the station after it.
        case Phase::Arriving:
            if (m_timing.workMs == kContinuous) {
                return kContinuous;
            }
            total = m_timing.transitMs + m_timing.workMs + m_timing.clearMs;
            break;
        case Phase::Working:
            if (m_timing.workMs == kContinuous) {
                return kContinuous;
            }
            total = m_timing.workMs + m_timing.clearMs;
            break;
        case Phase::Clearing:
            total = m_timing.clearMs;
            break;
    }
    uint32_t elapsedMs = elapsed(clock);
    return elapsedMs >= total ? 0 : total - elapsedMs;
}

bool Station::freeWithin(uint32_t clock, uint32_t slackMs) const {
    return ready() && remainingMs(clock) <= slackMs;
}

bool Station::activate(uint32_t clock) {
    if (!free()) {
        return false;
    }
    enter(Phase::Arriving, clock);
    onActivate();
    logLine("%s: activated", m_name.c_str());
    return true;
}

void Station::deactivate(uint32_t clock) {
    if (m_phase == Phase::Idle || m_phase == Phase::Clearing) {
        return;
    }
    onRelease();
    // A station with clearMs 0 returns straight to Idle.
    enter(m_timing.clearMs ? Phase::Clearing : Phase::Idle, clock);
    logLine("%s: released", m_name.c_str());
}

void Station::reset() {
    onReset();
    enter(Phase::Idle, m_since);
}

std::string Station::state() const {
    static const char* kPhaseNames[] = {"idle", "arriving", "working", "done", "clearing"};
    std::string text = kPhaseNames[static_cast<int>(m_phase)];
    std::string extra = detail();
    return extra.empty() ? text : text + ", " + extra;
}

void Station::enter(Phase phase, uint32_t clock) {
    m_phase = phase;
    m_since = clock;
    m_stallReported = false;
}
