#ifndef OVEN_H
#define OVEN_H

#include <P1AM.h>

#include "machine/Station.h"

// Holds a tray for cookMs with the heater relay on. The tray hold solenoid
// rests captured for that window and energizes to release the tray
// afterward. The thermistor reading appears in the status line.
class Oven : public Station {
   public:
    struct Config {
        bool enabled;             // True drives the heater, hold and thermistor
        channelLabel heater;      // Heater relay
        channelLabel hold;        // Tray hold solenoid; energize to release, rests captured
        channelLabel thermistor;  // Shown in status
        uint32_t cookMs;          // Time the tray is held in the oven
        uint32_t transitMs;       // Upstream release -> tray arrives here
        uint32_t clearMs;         // Release -> tray fully past this station
    };

    Oven(std::string name, P1AM& p1, Config config);

    void selfTest() override;

    // Cuts a cook short so the tray moves on immediately, as if cookMs had
    // elapsed. True if a cook was in progress; false (ignored) otherwise.
    bool cancelCook();

   protected:
    void poll() override;
    void onActivate() override;
    void onArrive() override;
    bool onWork(uint32_t elapsedMs) override;
    void onComplete() override;
    void onRelease() override;
    void onReset() override;

    std::string detail() const override;

   private:
    static Timing timingFor(const Config& config);
    void setHeating(bool on);
    void setHeld(bool held);

    P1AM& m_p1;
    Config m_config;
    float m_temperature = 0;
    bool m_working = false;  // In the Working phase of a real cook cycle
    bool m_cancelRequested = false;
};

#endif
