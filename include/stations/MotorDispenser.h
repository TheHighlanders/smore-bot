#ifndef MOTORDISPENSER_H
#define MOTORDISPENSER_H

#include <P1AM.h>

#include "machine/Station.h"

// Runs the motor from the moment the tray arrives until the exit sensor
// confirms the marshmallow has left, then stops. timeoutMs is a safety bound
// only, in case the sensor never triggers.
class MotorDispenser : public Station {
   public:
    struct Config {
        channelLabel capture;     // Tray stop
        channelLabel motor;       // Energize to run the dispenser motor
        channelLabel exitSensor;  // Light sensor: triggers once product has exited
        uint32_t timeoutMs;       // Safety bound if the sensor never triggers
        uint32_t transitMs;       // Upstream release -> tray arrives here
        uint32_t clearMs;         // Release -> tray fully past this station
    };

    MotorDispenser(std::string name, P1AM& p1, Config config);

    void selfTest() override;

   protected:
    void onActivate() override;
    void onArrive() override;
    bool onWork(uint32_t elapsedMs) override;
    void onComplete() override;
    void onRelease() override;
    void onEStop() override;

   private:
    static Timing timingFor(const Config& config);
    void setRunning(bool running);

    P1AM& m_p1;
    Config m_config;
    bool m_running = false;
};

#endif
