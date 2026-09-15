#ifndef OVEN_H
#define OVEN_H

#include <P1AM.h>

#include "machine/Station.h"

// Holds a tray for cookMs with the heater relay and tray hold solenoid on
// together for that window. The thermistor reading appears in the status line.
class Oven : public Station {
   public:
    struct Config {
        bool enabled;             // True drives the heater, hold and thermistor
        channelLabel heater;      // Heater relay
        channelLabel hold;        // Tray hold solenoid
        channelLabel thermistor;  // Shown in status
        uint32_t cookMs;          // Time the tray is held in the oven
        uint32_t transitMs;       // Upstream release -> tray arrives here
        uint32_t clearMs;         // Release -> tray fully past this station
    };

    Oven(std::string name, P1AM& p1, Config config);

    void selfTest() override;

   protected:
    void poll() override;
    void onActivate() override;
    void onRelease() override;
    void onReset() override;

    std::string detail() const override;

   private:
    static Timing timingFor(const Config& config);
    void setHeating(bool on);

    P1AM& m_p1;
    Config m_config;
    float m_temperature = 0;
};

#endif
