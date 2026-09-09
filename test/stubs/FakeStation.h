#pragma once

#include <string>
#include <vector>

#include "machine/Station.h"

// Records its own lifecycle and fails the test if the machine ever puts two
// trays in it at once.
class FakeStation : public Station {
   public:
    FakeStation(std::string name, Timing timing) : Station(name, timing) {}

    void selfTest() override {}

    int activations = 0;
    int completions = 0;
    int works = 0;
    uint32_t lastWorkElapsed = 0;
    bool occupied = false;
    bool collided = false;
    bool safed = false;

    // Stands in for a gate like the oven's "at temperature".
    bool readyGate = true;

    // Stands in for a sensor like MM's exit sensor: set true to make onWork()
    // report completion immediately, before workMs elapses.
    bool workDone = false;

    std::vector<std::string> events;

   protected:
    bool ready() const override { return readyGate; }

    void onActivate() override {
        if (occupied) collided = true;
        occupied = true;
        activations++;
        events.push_back("activate");
    }
    void onArrive() override { events.push_back("arrive"); }
    bool onWork(uint32_t elapsedMs) override {
        works++;
        lastWorkElapsed = elapsedMs;
        return workDone;
    }
    void onComplete() override {
        completions++;
        events.push_back("complete");
    }
    void onRelease() override {
        occupied = false;
        events.push_back("release");
    }
    void onEStop() override {
        occupied = false;
        safed = true;
    }
};
