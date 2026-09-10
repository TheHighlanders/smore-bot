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

// Base slot numbers. Every channelLabel below names one of these instead of
// a bare number, so moving a module in kModules is a one-line change here
// instead of a hunt through every channel that pointed at its old slot.
enum Slot : uint8_t {
    kSlotThermistor = 1,
    kSlotDiscreteIn = 2,
    kSlotAnalogIn = 3,
    kSlotDiscreteOut = 4,
    kSlotRelay = 5,
};

struct ModuleSlot {
    const char* name;
    uint8_t slot;  // Slots are 1-indexed
};

// Expected base layout, verified against the base controller at boot. Every
// module listed here must be present or the machine refuses to start.
const ModuleSlot kModules[] = {
    {"P1-04NTC", kSlotThermistor},   // Oven temperature
    {"P1-16ND3", kSlotDiscreteIn},   // Start button, MM exit sensor
    {"P1-04AD-2", kSlotAnalogIn},    // Spare
    {"P1-15TD2", kSlotDiscreteOut},  // Pneumatic gates, conveyor, linear actuators
    {"P1-08TRS", kSlotRelay},        // Oven heater, MM motor
};
const size_t kModuleCount = sizeof(kModules) / sizeof(kModules[0]);

// Thermistor module: high-side burnout, degF, 10k-CP (type 3), all channels on.
const char kThermistorSetup[] = {0x40, 0x03, 0x60, 0x07, 0x20, 0x02, 0x80, 0x00};

const channelLabel kStartButton = {kSlotDiscreteIn, 10};
// The e-stop is hardware now: it cuts power directly, so there is no channel
// for software to read or act on.

// Discrete out (kSlotDiscreteOut, P1-15TD2) channel map:
//   1     GC2 claw
//   2     GC2 lifter
//   3     MM capture
//   4     unused
//   5     GC2 capture
//   6     GC1 capture (not wired yet)
//   7     CHOC capture (not wired yet)
//   8-11  TBD
//   12    conveyor motor
//   13    GC2 pusher
//   14    CHOC linear actuator
//   15    GC1 linear actuator
//
// Relay (kSlotRelay, P1-08TRS) channel map:
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
    .capture = {kSlotDiscreteOut, 6},  // Not wired yet; placeholder to stay clear of GC2
    .extend = {kSlotDiscreteOut, 15},
    .extendMs = 3000,
    .dwellMs = 600,
    .retractMs = 3000,
    .transitMs = 0,  // Entry station: nothing upstream to travel from
    .clearMs = 2000,
};

const LinearDispenser::Config kChocolate = {
    .capture = {kSlotDiscreteOut, 7},  // Not wired yet; placeholder to stay clear of GC2
    .extend = {kSlotDiscreteOut, 14},
    .extendMs = 3000,
    .dwellMs = 600,
    .retractMs = 3000,
    .transitMs = 3000,
    .clearMs = 2000,
};

const MotorDispenser::Config kMarshmallow = {
    .capture = {kSlotDiscreteOut, 3},
    .motor = {kSlotRelay, 8},
    .exitSensor = {kSlotDiscreteIn, 9},
    .timeoutMs = 8000,  // CALIBRATE: safety bound if the sensor never triggers
    .transitMs = 3000,
    .clearMs = 2000,
};

// The sequence itself (states + durations) lives in GcPusher.cpp, not here.
const GcPusher::Config kGrahamCracker2 = {
    .capture = {kSlotDiscreteOut, 5},
    .lifter = {kSlotDiscreteOut, 2},
    .claw = {kSlotDiscreteOut, 1},
    .pusher = {kSlotDiscreteOut, 13},
    .transitMs = 3000,
    .clearMs = 2000,
};

// Not under test right now: the oven never touches its hardware and always
// reports ready, so the rest of the line can run without it.
const bool kOvenEnabled = true;

// CALIBRATE the setpoint too. 85F is roughly ambient, so as shipped the heater
// never fires and the oven reports ready immediately.
const Oven::Config kOven = {
    .enabled = kOvenEnabled,
    .heater = {kSlotRelay, 1},
    .hold = {kSlotDiscreteOut, 8},  // TBD, unconfirmed: disabled above, so unused
    .thermistor = {kSlotThermistor, 1},
    .setpointF = 85.0f,
    .deadbandF = 1.0f,
    .cookMs = 45000,
    .transitMs = 3000,
    .clearMs = 2500,
};

const channelLabel kConveyorMotor = {kSlotDiscreteOut, 12};

// Watchdog window. HOLD de-energizes every module output and stops the CPU
// until a power cycle, so a hung sketch cannot leave the heater on.
const uint16_t kWatchdogMs = 5000;

}  // namespace config

#endif
