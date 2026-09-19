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
    for (Station* station : m_continuous) {
        station->update(now);
    }
    for (Station* station : m_line) {
        station->update(now);
    }

    if (!m_running) {
        return;
    }

    // Downstream first, so each free() check sees the moves already made further
    // down the line this tick.
    for (size_t i = m_line.size(); i-- > 0;) {
        Station* station = m_line[i];
        if (!station->done()) {
            continue;
        }

        if (i + 1 == m_line.size()) {
            station->deactivate(now);
            logLine("Cycle complete");
        } else if (m_line[i + 1]->free()) {
            station->deactivate(now);
            m_line[i + 1]->activate(now);
        }
    }
}

bool Machine::canStart() const {
    return m_running && !m_line.empty() && m_line.front()->free();
}

bool Machine::startCycle() {
    return canStart() && m_line.front()->activate(millis());
}

void Machine::run(bool enable) {
    if (enable == m_running) {
        return;
    }
    m_running = enable;

    if (enable) {
        for (Station* station : m_continuous) {
            station->activate(millis());
        }
    } else {
        for (Station* station : m_continuous) {
            station->reset();
        }
        for (Station* station : m_line) {
            station->reset();
        }
    }
    logLine("Machine %s", enable ? "running" : "stopped, stations reset");
}

void Machine::printStatus() const {
    logLine("Machine: %s", m_running ? "running" : "stopped");
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
