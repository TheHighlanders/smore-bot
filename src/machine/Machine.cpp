#include "machine/Machine.h"

#include <Arduino.h>
#include <ctype.h>

#include "Log.h"

void Machine::configure(std::vector<Station*> line, std::vector<Station*> continuous) {
    m_line = line;
    m_continuous = continuous;
}

void Machine::update() {
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
            logLine("Cycle complete");
        } else if (m_line[i + 1]->free()) {
            station->deactivate(m_clock);
            m_line[i + 1]->activate(m_clock);
        }
    }
}

bool Machine::startCycle() {
    if (!m_running || m_line.empty()) {
        return false;
    }
    return m_line.front()->activate(m_clock);
}

void Machine::run(bool enable) {
    if (enable == m_running) {
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
    logLine("Machine %s", enable ? "running" : "held");
}

void Machine::printStatus() const {
    logLine("Machine: %s, belt clock %lus", m_running ? "running" : "held",
              (unsigned long)(m_clock / 1000));
    for (Station* station : m_line) {
        logLine("\t%s: %s", station->name().c_str(), station->state().c_str());
    }
    for (Station* station : m_continuous) {
        logLine("\t%s: %s", station->name().c_str(), station->state().c_str());
    }
}

namespace {
std::string toLower(std::string s) {
    for (char& c : s) {
        c = tolower(static_cast<unsigned char>(c));
    }
    return s;
}
}  // namespace

bool Machine::selfTestNamed(const char* name) const {
    std::string target = toLower(name);
    for (Station* station : m_line) {
        if (toLower(station->name()) == target) {
            station->selfTest();
            return true;
        }
    }
    for (Station* station : m_continuous) {
        if (toLower(station->name()) == target) {
            station->selfTest();
            return true;
        }
    }
    return false;
}
