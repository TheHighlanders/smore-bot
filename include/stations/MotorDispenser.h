#ifndef MOTORDISPENSER_H
#define MOTORDISPENSER_H

#include <P1AM.h>

#include "machine/Station.h"

// Runs the motor from the moment the tray arrives until the exit sensor
// confirms the marshmallow has left: it must first read blocked (the
// marshmallow reaching the sensor), then read clear for kDebounceMs straight
// (fully exited, not a momentary flicker). timeoutMs is a safety bound only,
// in case the sensor never triggers at all.
class MotorDispenser : public Station {
   public:
    struct Config {
        channelLabel capture;     // Tray stop
        channelLabel motor;       // Energize to run the dispenser motor
        channelLabel exitSensor;  // Light sensor: reads blocked while product is in it
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
    static const uint32_t kDebounceMs = 100;  // Clear must hold this long to count

    static Timing timingFor(const Config& config);
    void setRunning(bool running);

    P1AM& m_p1;
    Config m_config;
    bool m_running = false;
    bool m_seenBlocked = false;   // The beam has been broken at least once
    bool m_clearing = false;      // Currently timing a debounced clear
    uint32_t m_clearSinceMs = 0;  // elapsedMs when the clear started
};

#endif
