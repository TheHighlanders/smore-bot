#pragma once

#include <string>

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
    bool occupied = false;
    bool collided = false;
    bool safed = false;

   protected:
    void onActivate() override {
        if (occupied) collided = true;
        occupied = true;
        activations++;
    }
    void onWork(uint32_t) override { works++; }
    void onComplete() override { completions++; }
    void onRelease() override { occupied = false; }
    void onEStop() override {
        occupied = false;
        safed = true;
    }
};
