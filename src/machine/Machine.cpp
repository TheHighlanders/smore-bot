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
            for (Station* station : heldStations) {
                // These stations were not free to advance when they finished,
                // they should be rechecked
                Station* next = getNextStation(station);
                if (next && next->free()) {
                    next->activate(this);
                    station->deactivate();
                }
            }
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
        if (nextStation->free() && nextStation->activate(this)) {
            station->deactivate();
            return;
        }

        heldStations.push_back(station);
    } else {
        station->deactivate();
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

const std::vector<Station*>* Machine::getStations() { return &stations; }

const std::vector<Station*>* Machine::getContinuousStations() { return &continuousStations; }
