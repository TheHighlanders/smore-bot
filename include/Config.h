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
    {"P1-16ND3", 1},   // Discrete in: start button, MM exit sensor
    {"P1-04NTC", 2},   // Thermistor: oven temperature
    {"P1-04AD-2", 3},  // Analog in: spare
    {"P1-15TD2", 4},   // Discrete out: pneumatic gates, conveyor, linear actuators
    {"P1-08TRS", 5},   // Relay: oven heater, MM motor
};
const size_t kModuleCount = sizeof(kModules) / sizeof(kModules[0]);

// Thermistor module: high-side burnout, degF, 10k-CP (type 3), all channels on.
const char kThermistorSetup[] = {0x40, 0x03, 0x60, 0x07, 0x20, 0x02, 0x80, 0x00};

const channelLabel kStartButton = {1, 10};
// The e-stop is hardware now: it cuts power directly, so there is no channel
// for software to read or act on.

// Discrete out (slot 4, P1-15TD2) channel map:
//   1-5   capture gates, in line order: GC1, CHOC, MM, OVEN, GC2
//   6     GC2 grab (gripper)
//   7     GC2 lift (raise/lower)
//   8-11  TBD
//   12    conveyor motor
//   13-15 linear actuators: GC1, CHOC, GC2 push, in that order (ASSUMED -
//         bring-up triggers one station at a time, so a swap shows up fast)
//
// Relay (slot 5, P1-08TRS) channel map:
//   1  oven heater
//   2  spare; a lightbulb stands in for it on the bench right now
//   8  MM dispense motor

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
    .extend = {4, 13},
    .extendMs = 700,
    .dwellMs = 400,
    .retractMs = 700,
    .transitMs = 0,  // Entry station: nothing upstream to travel from
    .clearMs = 2000,
};

const LinearDispenser::Config kChocolate = {
    .capture = {4, 2},
    .extend = {4, 14},
    .extendMs = 700,
    .dwellMs = 400,
    .retractMs = 700,
    .transitMs = 3000,
    .clearMs = 2000,
};

const MotorDispenser::Config kMarshmallow = {
    .capture = {4, 3},
    .motor = {5, 8},
    .exitSensor = {1, 9},
    .timeoutMs = 5000,  // CALIBRATE: safety bound if the sensor never triggers
    .transitMs = 3000,
    .clearMs = 2000,
};

const GcPusher::Config kGrahamCracker2 = {
    .capture = {4, 5},
    .push = {4, 15},
    .grab = {4, 6},
    .lift = {4, 7},
    .pushMs = 700,
    .lowerMs = 400,
    .grabMs = 300,
    .raiseMs = 400,
    .releaseMs = 300,
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
    .heater = {5, 1},
    .hold = {4, 8},  // TBD, unconfirmed: oven is disabled above, so unused
    .thermistor = {2, 1},
    .setpointF = 85.0f,
    .deadbandF = 1.0f,
    .cookMs = 45000,
    .transitMs = 3000,
    .clearMs = 2500,
};

const channelLabel kConveyorMotor = {4, 12};

// Watchdog window. HOLD de-energizes every module output and stops the CPU
// until a power cycle, so a hung sketch cannot leave the heater on.
const uint16_t kWatchdogMs = 5000;

}  // namespace config

#endif
