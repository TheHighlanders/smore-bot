#ifndef MOTORDISPENSER_H
#define MOTORDISPENSER_H

#include <P1AM.h>

#include "machine/Station.h"

// Runs the motor from tray arrival until the exit sensor reads blocked and then
// clear for kDebounceMs. timeoutMs caps the run if the sensor stays quiet.
class MotorDispenser : public Station {
   public:
    struct Config {
        channelLabel capture;     // Tray stop; energize to release, rests captured
        channelLabel motor;       // Energize to run the dispenser motor
        channelLabel exitSensor;  // Light sensor: reads blocked while product is in it
        uint32_t timeoutMs;       // Maximum motor run
        uint32_t transitMs;       // Upstream release -> tray arrives here
        uint32_t clearMs;         // Release -> tray fully past this station
    };

    MotorDispenser(std::string name, P1AM& p1, Config config);

    void selfTest() override;

   protected:
    void poll() override;
    void onActivate() override;
    void onArrive() override;
    bool onWork(uint32_t elapsedMs) override;
    void onComplete() override;
    void onRelease() override;
    void onReset() override;

   private:
    static const uint32_t kDebounceMs = 100;  // Clear must hold this long to count

    static Timing timingFor(const Config& config);
    void setRunning(bool running);
    void setCaptured(bool captured);

    P1AM& m_p1;
    Config m_config;
    bool m_running = false;
    bool m_seenBlocked = false;   // The beam has been broken this cycle
    bool m_clearing = false;      // Timing a debounced clear
    uint32_t m_clearSinceMs = 0;  // elapsedMs when the clear started
    bool m_captured = true;  // Outputs power up de-energized, which now rests captured
    uint32_t m_releasedAt = 0;
};

#endif
