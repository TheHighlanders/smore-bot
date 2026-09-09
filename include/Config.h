#ifndef CONFIG_H
#define CONFIG_H

#include <P1AM.h>

#include "machine/Station.h"
#include "stations/GcPusher.h"
#include "stations/LinearDispenser.h"
#include "stations/MotorDispenser.h"
#include "stations/Oven.h"

// Every hardware address and dead-reckoning time lives here. Included once,
// from Rig.cpp, where the stations are built and the line order is set.
namespace config {

struct ModuleSlot {
    const char* name;
    uint8_t slot;  // Slots are 1-indexed
};

// Expected base layout, verified against the base controller at boot. Every
// module listed here must be present or the machine refuses to start.
const ModuleSlot kModules[] = {
    {"P1-16ND3", 1},   // Discrete in: start and e-stop buttons
    {"P1-04NTC", 2},   // Thermistor: oven temperature
    {"P1-04AD-2", 3},  // Analog in: spare
    {"P1-15TD2", 4},   // Discrete out: capture solenoids
    {"P1-08TRS", 5},   // Relay: belt and oven heater
};
const size_t kModuleCount = sizeof(kModules) / sizeof(kModules[0]);

// Thermistor module: high-side burnout, degF, 10k-CP (type 3), all channels on.
const char kThermistorSetup[] = {0x40, 0x03, 0x60, 0x07, 0x20, 0x02, 0x80, 0x00};

const channelLabel kEStopButton = {1, 9};
const channelLabel kStartButton = {1, 10};

// Dead-reckoned times, in milliseconds of belt motion. Every station carries
// its own, so they can be tuned one at a time:
//
//   transitMs  release upstream -> tray reaches this station's stop
//   clearMs    release -> tray fully past this station, which is the only
//              interlock protecting the station behind it
//
// Work duration is never set directly: each station sums its own actuator
// sequence, so timing cannot drift out of step with the moves performed.
//
// CALIBRATE all of these against the real belt before running product.

const LinearDispenser::Config kGrahamCracker1 = {
    .capture = {4, 1},
    .extend = {4, 2},
    .extendMs = 700,
    .dwellMs = 400,
    .retractMs = 700,
    .transitMs = 0,  // Entry station: nothing upstream to travel from
    .clearMs = 2000,
};

const LinearDispenser::Config kChocolate = {
    .capture = {4, 3},
    .extend = {4, 4},
    .extendMs = 700,
    .dwellMs = 400,
    .retractMs = 700,
    .transitMs = 3000,
    .clearMs = 2000,
};

// TBD hardware: assumes a hobby motor switched by a discrete output.
const MotorDispenser::Config kMarshmallow = {
    .capture = {4, 5},
    .motor = {4, 6},
    .runMs = 1200,
    .settleMs = 600,
    .transitMs = 3000,
    .clearMs = 2000,
};

const GcPusher::Config kGrahamCracker2 = {
    .capture = {4, 8},
    .gripper = {4, 9},
    .lift = {4, 10},
    .translate = {4, 11},
    // grab, lift, translate, lower, release, return
    .moveMs = {400, 600, 900, 600, 400, 900},
    .transitMs = 3000,
    .clearMs = 2000,
};

// Not under test right now: the oven never touches its hardware and always
// reports ready, so the rest of the line can run without it.
const bool kOvenEnabled = false;

// CALIBRATE the setpoint too. 85F is roughly ambient, so as shipped the heater
// never fires and the oven reports ready immediately.
const Oven::Config kOven = {
    .enabled = kOvenEnabled,
    .heater = {5, 2},
    .hold = {4, 7},
    .thermistor = {2, 1},
    .setpointF = 85.0f,
    .deadbandF = 1.0f,
    .cookMs = 45000,
    .transitMs = 3000,
    .clearMs = 2500,
};

const channelLabel kBeltRelay = {5, 1};

// Watchdog window. HOLD de-energizes every module output and stops the CPU
// until a power cycle, so a hung sketch cannot leave the heater on.
const uint16_t kWatchdogMs = 5000;

}  // namespace config

#endif
