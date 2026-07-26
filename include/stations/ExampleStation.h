#ifndef EXAMPLESTATION_h
#define EXAMPLESTATION_h

#include "machine/Machine.h"
#include "machine/Station.h"

class ExampleStation : public Station{
    public:
        ExampleStation(std::string name) : Station(name) {}; // Constructs an ExampleStation. Names must be unique.

        virtual void update() override; // Update the station (poll sensors, manage internal state, etc)

        virtual bool activate(Machine* machine) override; // Activates a station to do work. Return indicates if it was accepted or not, fires Machine callback when done.
        virtual void deactivate() override; // Returns the station to an inactive state.
        virtual bool free() const override; // Is the station available to do work. (IE, has it been deactivated, and confirmed it is clear (if applicable))

        virtual void eStop() override; // Safes all hardware connected to the station.

        virtual std::string state() const override;

    private:
        unsigned long activationTime = 0;
        unsigned long completionTime = 0;
};

#endif