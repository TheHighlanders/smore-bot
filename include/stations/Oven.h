#ifndef OVEN_H
#define OVEN_H

#include <P1AM.h>

#include "SerialBoolean.h"
#include "machine/Station.h"

// Holds a tray for the cook time. Only accepts work once it is at temperature.
class Oven : public Station {
   public:
    struct Config {
        channelLabel heater;      // Heater relay
        channelLabel hold;        // Tray hold solenoid
        channelLabel thermistor;  // Slot 0 to run without a temperature probe
        float setpointF;
        float deadbandF;
        uint32_t cookMs;     // Time the tray is held in the oven
        uint32_t transitMs;  // Upstream release -> tray arrives here
        uint32_t clearMs;    // Release -> tray fully past this station
    };

    Oven(std::string name, P1AM& p1, Config config);

    void selfTest() override;

   protected:
    void poll(bool machineRunning) override;
    void onActivate() override;
    void onRelease() override;
    void onEStop() override;

    bool ready() const override { return m_atTemp; }
    std::string detail() const override;

   private:
    static Timing timingFor(const Config& config);
    void setAtTemp(bool value);

    P1AM& m_p1;
    Config m_config;

    float m_temperature = 0;
    bool m_atTemp = false;

    SerialBoolean m_forceAtTemp;  // "<name>temp" toggles the probe out of the loop
};

#endif
