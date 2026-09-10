#ifndef MACHINE_H
#define MACHINE_H

#include <stdint.h>

#include <vector>

#include "machine/Station.h"

// Orchestrates an ordered line of stations plus continuous ones (the belt).
//
// Stations are polled, never called back, so a finished station can only be
// advanced once per tick and cannot re-enter the machine while advancing.
class Machine {
   public:
    void configure(std::vector<Station*> line, std::vector<Station*> continuous);

    void update();
    bool startCycle();

    void run(bool enable);  // Enable/hold work assignment

    bool isRunning() const { return m_running; }

    void printStatus() const;

    // Bring-up: run one station's selfTest by name, case-insensitive. False if
    // no such station.
    bool selfTestNamed(const char* name) const;

   private:
    std::vector<Station*> m_line;
    std::vector<Station*> m_continuous;

    // Belt clock. Advances only while the machine runs, so dead-reckoned
    // timings stay aligned with actual tray travel across a hold.
    uint32_t m_clock = 0;
    uint32_t m_lastTick = 0;

    bool m_running = false;
};

#endif
