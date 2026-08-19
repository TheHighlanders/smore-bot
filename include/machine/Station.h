#ifndef STATION_h
#define STATION_h

#include "machine/Machine.h"
#include "Print.h"

#include <string>
#include <functional>

class Machine;

// Virtual implementation for extending for various stations
class Station{
    public:
        Station(std::string p_name) : active(false), m_name(p_name) {}; // Constructs a new station. Station names must be unique

        virtual std::string name() const {return m_name;}; // Get station name
        virtual std::string state() const = 0; // Get current station state

        virtual void update() = 0; // Update the station (poll sensors, manage internal state, etc)

        virtual bool activate(Machine* machine) = 0; // Activates a station to do work. Return indicates if it was accepted or not, fires Machine callback when done.
        virtual void deactivate() = 0; // Returns the station to an inactive state.
        virtual bool free() const = 0; // Is the station available to do work. (IE, has it been deactivated, and confirmed it is clear (if applicable))

        virtual void eStop() = 0; // Safes all hardware connected to the station.
    
        bool operator==(const Station* other) const {return name() == other->name();};

    protected:
        bool active;
        std::string m_name;
        Machine* m_machine; // Machine to notify when work is complete
};
#endif