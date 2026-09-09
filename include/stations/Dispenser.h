#ifndef DISPENSER_H
#define DISPENSER_H

#include <P1AM.h>

#include "machine/Station.h"

// Captures a tray with a solenoid stop, runs the dispenser servo, releases.
class Dispenser : public Station {
   public:
    struct Config {
        channelLabel capture;   // Solenoid stop that holds the tray
        uint8_t servoPin;       // Dispenser servo PWM pin
        uint8_t servoParked;    // Duty while not dispensing
        uint8_t servoDispense;  // Duty while dispensing
    };

    Dispenser(std::string name, P1AM& p1, Config config, Timing timing);

   protected:
    void onActivate() override;
    void onArrive() override;
    void onComplete() override;
    void onRelease() override;
    void onEStop() override;

   private:
    P1AM& m_p1;
    Config m_config;
};

#endif
