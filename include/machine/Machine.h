#ifndef MACHINE_H
#define MACHINE_H

#include <stdint.h>

#include <vector>

#include "machine/Station.h"

// Runs an ordered line of stations plus continuous ones (the belt). Each tick it
// updates every station, then moves finished trays down the line.
class Machine {
   public:
    void configure(std::vector<Station*> line, std::vector<Station*> continuous);

    void update();
    bool startCycle();

    // On: start the continuous stations and accept cycles. Off: reset every
    // station to Idle with outputs de-energized, ready for the operator to
    // clear the belt.
    void run(bool enable);

    bool isRunning() const { return m_running; }

    void printStatus() const;

    // Bring-up: run one station's selfTest by name, case-insensitive. True if a
    // station matched.
    bool selfTestNamed(const char* name) const;

   private:
    std::vector<Station*> m_line;
    std::vector<Station*> m_continuous;
    bool m_running = false;
};

#endif
