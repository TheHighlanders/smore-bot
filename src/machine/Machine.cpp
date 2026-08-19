#include "machine/Machine.h"

#include <algorithm>

#include "machine/Station.h"

Timer<5, millis> Machine::timer;  // Define and allocate the timer.

void Machine::update() {
    if (!eStopped) {
        // Update all stations
        for (Station* station : stations) {
            station->update();
        }
        for (Station* station : continuousStations){
            station->update();
        }

        if (running) {
            std::vector<Station*> stillHeldStations;
            for (Station* station : heldStations) {
                // These stations were not free to advance when they finished,
                // they should be rechecked
                logUpdate("Update: Machine: Retrying Held Station %s", station->name().c_str());
                Station* next = getNextStation(station);
                if (next && next->free()) {
                    if(next->activate(this)){
                        station->deactivate();
                        logInfo("\tSuccess");
                        continue;
                    }
                    logError("\tUnable to Activate Next");
                } else {
                    logError("\tNext Station Not Free");
                }
                stillHeldStations.push_back(station);
            }
            heldStations = stillHeldStations;
        }
    }
}

bool Machine::startCycle() {
    if (running && !eStopped) {
        return stations[0]->activate(this);
    }
    return false;
}

void Machine::stop() {if(running){
    for(auto station : continuousStations){
        station->deactivate();
    }
    running = false; 
}}

void Machine::resume() {
    if (!running) {
        for(auto station : continuousStations){
            station->activate(this);
        }
        running = true;
    }
}

void Machine::eStop() {
    eStopped = true;
    running = false;
    for (Station* station : stations) {
        station->eStop();
    }
    for(Station* station : continuousStations){
        station->eStop();
    }
}

void Machine::onWorkCompleteCallback(Station* station) {
    Station* nextStation = getNextStation(station);
    if (nextStation) {
        if(!nextStation->free()){
            heldStations.push_back(station);
        } else {
            if(!nextStation->activate(this)){
                heldStations.push_back(station);
            } else {
                station->deactivate();
            }
        }
    } else {
        station->deactivate();
        logUpdate("Cycle Complete");
    }
}

int Machine::addStation(Station* station) {
    stations.push_back(station);
    return stations.size() - 1;
}

void Machine::addStation(Station* station, int index) {
    stations.insert(stations.begin() + index, station);
}

int Machine::addContinuousStation(Station* station) {
    continuousStations.push_back(station);
    return continuousStations.size() - 1;
}

void Machine::addContinuousStation(Station* station, int index) {
    continuousStations.insert(continuousStations.begin() + index, station);
}

Station* Machine::getNextStation(Station* station) {
    if (!station) {
        return nullptr;
    }
    auto it = std::find(stations.begin(), stations.end(), station);
    if (it != stations.end()) {
        auto next = std::next(it);
        if (next != stations.end()) {
            return *next;
        }
    }
    return nullptr;
}

void Machine::printStatus(){
    std::vector<Station*> stations = *getStations();
    std::vector<Station*> contStations = *getContinuousStations();
    logUpdate("Status Report:");
    logUpdate("Machine Status:");
    logInfo("\t%s, Linear Stations: %d, Continuous Stations: %d%s", (isRunning() ? "Running" : "Not Running"), (getStations()->size()), (getContinuousStations()->size()), (isEmergencyStopped() ? ", E-Stopped" : ""));
    logUpdate("Station Updates:");
    if(stations.size()){
        for(auto station : stations){
            logInfo("\t%s: %s", station->name().c_str(), station->state().c_str());
        }
    }
    if(contStations.size()){
        for(auto station : contStations){
            logInfo("\t%s: %s", station->name().c_str(), station->state().c_str());
        }
    }

}

const std::vector<Station*>* Machine::getStations() { return &stations; }

const std::vector<Station*>* Machine::getContinuousStations() { return &continuousStations; }
