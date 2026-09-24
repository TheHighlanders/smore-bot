#ifndef CONFIG_H
#define CONFIG_H

#include <P1AM.h>

#include "leds/StatusLeds.h"
#include "machine/Station.h"
#include "stations/GcPusher.h"
#include "stations/LinearDispenser.h"
#include "stations/MotorDispenser.h"
#include "stations/Oven.h"

// Every hardware address and time. Included once, from Rig.cpp, where the
// stations are built and the line order is set.
namespace config {

// Base slot numbers. Channels name slots through this enum, so moving a module
// is a one-line change here.
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

// Expected base layout, verified at boot. The machine starts once every module
// listed is present.
const ModuleSlot kModules[] = {
    {"P1-04NTC", kSlotThermistor},   // Oven temperature
    {"P1-16ND3", kSlotDiscreteIn},   // Start button, MM exit sensor
    {"P1-04AD-2", kSlotAnalogIn},    // Spare
    {"P1-15TD2", kSlotDiscreteOut},  // Pneumatic gates, conveyor, linear actuators
    {"P1-08TRS", kSlotRelay},        // Oven heater, ready light, MM motor
};
const size_t kModuleCount = sizeof(kModules) / sizeof(kModules[0]);

// Thermistor module: high-side burnout, degF, 10k-CP (type 3), all channels on.
const char kThermistorSetup[] = {0x40, 0x03, 0x60, 0x07, 0x20, 0x02, 0x80, 0x00};

const channelLabel kStartButton = {kSlotDiscreteIn, 11};
const channelLabel kCancelCookButton = {kSlotDiscreteIn, 12};  // Unwired until installed
const channelLabel kReadyLight = {kSlotRelay, 2};              // Lit when ready for a start

// Discrete in (kSlotDiscreteIn, P1-16ND3) channel map:
//   9   MM exit sensor
//   11  start button
//   12  cancel cook button

// Discrete out (kSlotDiscreteOut, P1-15TD2) channel map:
//   1     GC2 claw
//   2     GC2 lifter
//   3     MM capture
//   4     spare
//   5     GC2 capture
//   6     GC1 capture (planned)
//   7     CHOC capture (planned)
//   8-11  TBD
//   12    conveyor motor
//   13    GC2 pusher
//   14    CHOC linear actuator
//   15    GC1 linear actuator
//
// Relay (kSlotRelay, P1-08TRS) channel map:
//   1  oven heater
//   2  ready light
//   8  MM dispense motor

// Station times, in milliseconds:
//
//   transitMs  release upstream -> tray reaches this station's stop
//   clearMs    release -> tray fully past this station; the interlock that
//              protects the station behind it
//
// Each station's work duration is the sum of its own actuator times.
//
// CALIBRATE all of these against the real belt before running product.

const LinearDispenser::Config kGrahamCracker1 = {
    .capture = {kSlotDiscreteOut, 7},
    .extend = {kSlotDiscreteOut, 15},
    .extendMs = 7500,
    .retractMs = 7500,
    .transitMs = 0,  // Entry station
    .clearMs = 7000,
};

const LinearDispenser::Config kChocolate = {
    .capture = {kSlotDiscreteOut, 6},
    .extend = {kSlotDiscreteOut, 14},
    .extendMs = 4700,
    .retractMs = 4700,
    .transitMs = 5000,
    .clearMs = 7000,
};

const MotorDispenser::Config kMarshmallow = {
    .capture = {kSlotDiscreteOut, 5},
    .motor = {kSlotRelay, 8},
    .exitSensor = {kSlotDiscreteIn, 9},
    .timeoutMs = 3500,
    .transitMs = 5000,
    .clearMs = 7000,
};

// One GC2 cycle, in order.
const GcPusher::Move kGrahamCracker2Moves[] = {
    // lifterUp, clawClosed, pusherOut, ms
    {true, false, true, 7000},    // extend graham cracker
    {false, false, true, 2000},   // lower claw
    {false, true, true, 2000},    // close claw
    {true, true, false, 7000},    // raise claw, retract pusher
    {false, true, false, 2000},   // lower claw
    {false, false, false, 1000},  // open claw
    {true, false, false, 2000},   // raise claw
};

const GcPusher::Config kGrahamCracker2 = {
    .capture = {kSlotDiscreteOut, 3},
    .lifter = {kSlotDiscreteOut, 2},
    .claw = {kSlotDiscreteOut, 1},
    .pusher = {kSlotDiscreteOut, 13},
    .moves = kGrahamCracker2Moves,
    .moveCount = sizeof(kGrahamCracker2Moves) / sizeof(kGrahamCracker2Moves[0]),
    .transitMs = 7000,
    .clearMs = 5000,
};

// True runs the oven: heater and hold solenoid on for cookMs. False leaves its
// outputs and thermistor idle.
const bool kOvenEnabled = true;

const Oven::Config kOven = {
    .enabled = kOvenEnabled,
    .heater = {kSlotRelay, 1},
    .hold = {kSlotDiscreteOut, 4},
    .thermistor = {kSlotThermistor, 1},  // Shown in status
    .cookMs = 22 * 1000,
    .transitMs = 7000,
    .clearMs = 5000,
};

const channelLabel kConveyorMotor = {kSlotDiscreteOut, 12};

// Status LEDs: one strip along the belt, counted from the start of the belt.
// Each station lights its own range. The belt spans the whole strip and is drawn
// first, so the stations cover it and it shows on the LEDs left over.
//
// CALIBRATE the count and ranges against the real strip.
const uint16_t kLedCount = 60;

const StatusLeds::Range kBeltLeds = {0, kLedCount};  // First LED, LED count
const StatusLeds::Range kGrahamCracker1Leds = {0, 10};
const StatusLeds::Range kChocolateLeds = {10, 10};
const StatusLeds::Range kMarshmallowLeds = {20, 10};
const StatusLeds::Range kOvenLeds = {30, 10};
const StatusLeds::Range kGrahamCracker2Leds = {40, 10};

const Rgb kBlue = {0, 0, 255};
const Rgb kGreen = {0, 255, 0};
const Rgb kRed = {255, 0, 0};

// Every station shows:
//
//   idle      solid blue
//   arriving  green band, waiting for the tray
//   working   pulsing (see kPulse)
//   done      solid, in the pulse color
//   clearing  blue band, tray leaving
//
// A band moves from the station's first LED to its last.
const StatusLeds::Config kStatusLeds = {
    .idle = kBlue,
    .arriving = kGreen,
    .clearing = kBlue,
    .bandWidth = 3,
    .bandStepMs = 100,
};

// Working look: color, milliseconds per pulse.
const StatusLeds::Pulse kPulse = {kGreen, 2000};
const StatusLeds::Pulse kOvenPulse = {kRed, 800};

// Watchdog window. HOLD de-energizes every module output and halts the CPU
// until a power cycle, so a hung sketch turns the heater off.
const uint16_t kWatchdogMs = 5000;

}  // namespace config

#endif
