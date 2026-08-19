#ifndef BELT_h
#define BELT_h

#include "machine/Machine.h"
#include "machine/Station.h"
#include "P1AM.h"

class Belt : public Station{
    public:
        struct BeltHWConfig{
            channelLabel beltRelay;    // Output slot and channel with belt control relay connected
        };


        Belt(std::string name, P1AM& p1, BeltHWConfig hwConfig) : Station(name), hardware(p1), config(hwConfig){}; // Constructs a Belt station. Names must be unique.

        virtual void update() override; // Update the station (poll sensors, manage internal state, etc)

        virtual bool activate(Machine* machine) override; // Activates a station to do work. Return indicates if it was accepted or not, fires Machine callback when done.
        virtual void deactivate() override; // Returns the station to an inactive state.
        virtual bool free() const override; // Is the station available to do work. (IE, has it been deactivated, and confirmed it is clear (if applicable))

        virtual void eStop() override; // Safes all hardware connected to the station.

        virtual std::string state() const override;

    private:
        P1AM hardware;
        BeltHWConfig config;
};

#endif