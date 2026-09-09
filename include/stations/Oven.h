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
    };

    Oven(std::string name, P1AM& p1, Config config, Timing timing);

    void selfTest() override;

   protected:
    void poll(bool machineRunning) override;
    void onActivate() override;
    void onRelease() override;
    void onEStop() override;

    bool ready() const override { return m_atTemp; }
    std::string detail() const override;

   private:
    void setAtTemp(bool value);

    P1AM& m_p1;
    Config m_config;

    float m_temperature = 0;
    bool m_atTemp = false;

    SerialBoolean m_forceAtTemp;  // "<name>temp" toggles the probe out of the loop
};

#endif
