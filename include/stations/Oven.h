#ifndef OVEN_h
#define OVEN_h

#include "machine/Machine.h"
#include "machine/Station.h"
#include "P1AM.h"

class Oven : public Station{
    public:
        struct OvenHWConfig{
            channelLabel relaySolenoid;       // Output slot and channel with Oven heater relay connected
            channelLabel trayEntrySense;      // Input slot and channel with tray entry sensor connected
            channelLabel trayExitSense;       // Input slot and channel with tray exit sensor connected
            channelLabel trayholdSolenoid;    // Input slot and channel with tray hold solenoid connected
            channelLabel thermistor;          // Input slot and channle with thermistor connected
            float tempSetpoint;               // Setpoint for temperature of the oven
            float tempDeadzone;
        };

        // TODO: Config / setpoint / deadzone update live
        Oven(std::string name, P1AM& p1, OvenHWConfig hwConfig) : Station(name), hardware(p1), config(hwConfig), atTemp(false), trayInside(false) {} // Constructs a Oven station. Names must be unique.

        virtual void update() override; // Update the station (poll sensors, manage internal state, etc)

        virtual bool activate(Machine* machine) override; // Activates a station to do work. Return indicates if it was accepted or not, fires Machine callback when done.
        virtual void deactivate() override; // Returns the station to an inactive state.
        virtual bool free() const override; // Is the station available to do work. (IE, has it been deactivated, and confirmed it is clear (if applicable))

        virtual void eStop() override; // Safes all hardware connected to the station.

        virtual std::string state() const override;

    private:
        P1AM hardware;
        OvenHWConfig config;

        bool atTemp;
        bool trayInside;
};

#endif