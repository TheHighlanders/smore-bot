#ifndef TIMERSTATION_H
#define TIMERSTATION_H

#include "machine/Station.h"

// A station with no outputs. It holds a tray for its timing so the stations
// around it wait the right time, and it shows its phase on the LEDs. An
// operator does the work by hand.
class TimerStation : public Station {
   public:
    TimerStation(std::string name, Timing timing) : Station(name, timing) {}

    void selfTest() override;

   protected:
    void onReset() override {}
};

#endif
