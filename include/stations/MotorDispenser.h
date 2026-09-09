#ifndef MOTORDISPENSER_H
#define MOTORDISPENSER_H

#include <P1AM.h>

#include "machine/Station.h"

// DRAFT. Dispenses by running a motor for a fixed time, then waiting for the
// product to fall clear.
//
// TBD: assumes a hobby motor switched by one discrete output. If the motor
// needs more current than a DIO channel can source, only the channelLabel
// changes - point it at a relay instead.
class MotorDispenser : public Station {
   public:
    struct Config {
        channelLabel capture;  // Tray stop
        channelLabel motor;    // Energize to run the dispenser motor
        uint32_t runMs;        // Motor on
        uint32_t settleMs;     // Motor off, product falling
        uint32_t transitMs;    // Upstream release -> tray arrives here
        uint32_t clearMs;      // Release -> tray fully past this station
    };

    MotorDispenser(std::string name, P1AM& p1, Config config);

    void selfTest() override;

   protected:
    void onActivate() override;
    void onArrive() override;
    void onWork(uint32_t elapsedMs) override;
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
