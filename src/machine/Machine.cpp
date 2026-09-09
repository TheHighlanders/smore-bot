#include "machine/Machine.h"

#include <Arduino.h>

#include "Log.h"

void Machine::configure(std::vector<Station*> line, std::vector<Station*> continuous) {
    m_line = line;
    m_continuous = continuous;
}

void Machine::update() {
    if (m_eStopped) {
        return;
    }

    uint32_t now = millis();
    if (m_running) {
        m_clock += now - m_lastTick;
        m_lastTick = now;
    }

    for (Station* station : m_continuous) {
        station->update(m_clock, m_running);
    }
    for (Station* station : m_line) {
        station->update(m_clock, m_running);
    }

    if (!m_running) {
        return;
    }

    // Downstream first, so a tray never advances into a station that is itself
    // advancing this tick.
    for (size_t i = m_line.size(); i-- > 0;) {
        Station* station = m_line[i];
        if (!station->done()) {
            continue;
        }

        if (i + 1 == m_line.size()) {
            station->deactivate(m_clock);
            logUpdate("Cycle complete");
        } else if (m_line[i + 1]->free()) {
            station->deactivate(m_clock);
            m_line[i + 1]->activate(m_clock);
        }
    }
}

bool Machine::startCycle() {
    if (!m_running || m_eStopped || m_line.empty()) {
        return false;
    }
    return m_line.front()->activate(m_clock);
}

void Machine::run(bool enable) {
    if (m_eStopped || enable == m_running) {
        return;
    }
    m_running = enable;

    // Resume the clock from now so time spent held is not counted as travel.
    m_lastTick = millis();

    for (Station* station : m_continuous) {
        if (enable) {
            station->activate(m_clock);
        } else {
            station->deactivate(m_clock);
        }
    }
    logUpdate(enable ? "Machine running" : "Machine held");
}

void Machine::eStop() {
    m_eStopped = true;
    m_running = false;
    for (Station* station : m_line) {
        station->eStop(m_clock);
    }
    for (Station* station : m_continuous) {
        station->eStop(m_clock);
    }
}

void Machine::printStatus() const {
    logUpdate("Machine: %s%s, belt clock %lus", m_running ? "running" : "held",
              m_eStopped ? ", E-STOPPED" : "", (unsigned long)(m_clock / 1000));
    for (Station* station : m_line) {
        logInfo("\t%s: %s", station->name().c_str(), station->state().c_str());
    }
    for (Station* station : m_continuous) {
        logInfo("\t%s: %s", station->name().c_str(), station->state().c_str());
    }
}

void Machine::selfTest() const {
    for (Station* station : m_continuous) {
        station->selfTest();
    }
    for (Station* station : m_line) {
        station->selfTest();
    }
}
