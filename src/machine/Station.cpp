#include "machine/Station.h"

#include "Log.h"

// Open-loop machines have no jam feedback, so warn once if a finished station
// waits this long for the one downstream. Must exceed the total occupancy of
// the slowest station on the line, or a healthy full pipeline trips it.
static const uint32_t kStallWarnMs = 120000;

void Station::update(uint32_t clock, bool machineRunning) {
    poll(machineRunning);

    switch (m_phase) {
        case Phase::Arriving:
            if (elapsed(clock) >= m_timing.transitMs) {
                enter(Phase::Working, clock);
                onArrive();
            }
            break;

        case Phase::Working:
            if (m_timing.workMs != kContinuous && elapsed(clock) >= m_timing.workMs) {
                onComplete();
                enter(Phase::Done, clock);
                logInfo("%s: work complete", m_name.c_str());
            }
            break;

        case Phase::Done:
            if (!m_stallReported && elapsed(clock) >= kStallWarnMs) {
                m_stallReported = true;
                logError("%s: blocked %lus waiting on the next station",
                         m_name.c_str(), (unsigned long)(elapsed(clock) / 1000));
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

bool Station::activate(uint32_t clock) {
    if (!free()) {
        return false;
    }
    enter(Phase::Arriving, clock);
    onActivate();
    logInfo("%s: activated", m_name.c_str());
    return true;
}

void Station::deactivate(uint32_t clock) {
    if (m_phase == Phase::Idle || m_phase == Phase::Clearing) {
        return;
    }
    onRelease();
    // Skipping Clearing when there is nothing to clear keeps a station with no
    // clear time immediately reusable.
    enter(m_timing.clearMs ? Phase::Clearing : Phase::Idle, clock);
    logInfo("%s: released", m_name.c_str());
}

bool Station::forceComplete(uint32_t clock) {
    if (m_phase != Phase::Arriving && m_phase != Phase::Working) {
        return false;
    }
    onComplete();
    enter(Phase::Done, clock);
    logUpdate("%s: forced complete", m_name.c_str());
    return true;
}

void Station::eStop(uint32_t clock) {
    onEStop();
    enter(Phase::Idle, clock);
    logError("%s: E-STOPPED", m_name.c_str());
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
