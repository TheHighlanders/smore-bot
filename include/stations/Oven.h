#ifndef OVEN_H
#define OVEN_H

#include <P1AM.h>

#include "machine/Station.h"

// Holds a tray for cookMs, purely dead-reckoned. The heater relay is simply
// on whenever the machine is running - there is no setpoint or control loop.
// The thermistor is read every tick for the status line only; nothing ever
// gates on it, so a dead or unplugged probe cannot stall the line.
class Oven : public Station {
   public:
    struct Config {
        bool enabled;  // False: never touch the heater or hold solenoid
        channelLabel heater;      // Heater relay
        channelLabel hold;        // Tray hold solenoid
        channelLabel thermistor;  // Read-only; never gates anything
        uint32_t cookMs;          // Time the tray is held in the oven
        uint32_t transitMs;       // Upstream release -> tray arrives here
        uint32_t clearMs;         // Release -> tray fully past this station
    };

    Oven(std::string name, P1AM& p1, Config config);

    void selfTest() override;

   protected:
    void poll(bool machineRunning) override;
    void onActivate() override;
    void onRelease() override;

    std::string detail() const override;

   private:
    static Timing timingFor(const Config& config);

    P1AM& m_p1;
    Config m_config;
    float m_temperature = 0;
};

#endif
