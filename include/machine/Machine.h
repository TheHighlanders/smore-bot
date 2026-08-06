#ifndef MACHINE_h
#define MACHINE_h

#include "machine/Station.h"

#include <arduino-timer.h>
#include <vector>
#include <type_traits>

/**
 * Architecture:
 * Machine maintains a ordered list of stations, and is responsible for orchestrating the overall workflow
 * 
 * To do this, Machine will activate a station, wait for it to complete its work (and fire the completed callback)
 * Once work is done, Machine will determine if it is ready to proceed in the process (check next station)
 * To proceed, Machine deactivates the completed station, and activates the next one.
 */

class Machine{
    public:
        Machine() = default;
        Machine(std::vector<Station*> stationsVec) : stations(stationsVec) {};

        void update(); // Updates machine, and all component stations

        void onWorkCompleteCallback(Station* station); // Called by a component station when it has completed its work.

        int addStation(Station* station); //Adds a station, returns the index of that station
        void addStation(Station* station, int index); //Adds a station, at a specified index

        bool startCycle(); // Starts a new cycle, starting with stations[0];
        void resume(); // Enables further work assignment, without starting a new cycle
        void stop(); // Stops the machine from assigning any further work, without estopping stations. Holds the machine at end of current state

        void eStop(); // Estops all stations, and holds the top-level machine

        static Timer<5, millis> timer; //static timer object, max 5 concurrency, millisecond resolution

    private:
        Station* getNextStation(Station* station); // Returns ptr to the sequentially next station in the list.

        std::vector<Station*> stations; // All Stations in the machine

        std::vector<Station*> heldStations; // vector of stations that have completed work, but are currently unable to be deactivated

        /**
         * Running indicates whether stations should be assigned new work. Nothing will start while !running, but it can be disabled to hold the machine at a state
         * EStopped will call eStop() on all stations, and will stop updating stations.
         */
        bool running = false;
        bool eStopped = false;
};


#endif