#ifndef DISPENSER_h
#define DISPENSER_h

#include "machine/Machine.h"
#include "machine/Station.h"
#include "P1AM.h"

class Dispenser : public Station{
    public:
        struct DispenserHWConfig{
            channelLabel captureSolenoid;       // Output slot and channel with capture solenoid connected
            channelLabel dispensePWM;           // Output slot and channel with dispenser Servo connected
            channelLabel traySense;             // Input slot and channel with tray sensor connected
            float dispensePWMInactive;          // Position for dispenser servo while not dispensing
            float dispensePWMActive;            // Position for dispenser servo while dispensing
            long dispenseTimeMs;                // Time for dispense to complete once servo is fired
        };


        Dispenser(std::string name, P1AM& p1, DispenserHWConfig hwConfig) : Station(name), hardware(p1), config(hwConfig), dispensing(false) {}; // Constructs a Dispenser station. Names must be unique.

        virtual void update() override; // Update the station (poll sensors, manage internal state, etc)

        virtual bool activate(Machine* machine) override; // Activates a station to do work. Return indicates if it was accepted or not, fires Machine callback when done.
        virtual void deactivate() override; // Returns the station to an inactive state.
        virtual bool free() const override; // Is the station available to do work. (IE, has it been deactivated, and confirmed it is clear (if applicable))

        virtual void eStop() override; // Safes all hardware connected to the station.

        virtual std::string state() const override;

        static bool dispenseTimerCallback(void* argument); // Callback fired after dispense timer elapses, allowing dispense to occur

    private:
        P1AM hardware;
        DispenserHWConfig config;

        bool dispensing;
};

#endif