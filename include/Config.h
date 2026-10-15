#ifndef CONFIG_H
#define CONFIG_H

#include <P1AM.h>

#include "leds/StatusLeds.h"
#include "machine/Station.h"
#include "stations/GcPusher.h"
#include "stations/LinearDispenser.h"
#include "stations/MotorDispenser.h"
#include "stations/Oven.h"

// Every hardware address and time. Rig.cpp builds the stations and sets the line
// order from them.
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

// Expected base layout, checked at boot. If a module is missing or in the wrong
// slot, the sketch logs it and does not start.
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
const channelLabel kCancelCookButton = {kSlotDiscreteIn, 12};  // Cancel cook button
const channelLabel kReadyLight = {kSlotRelay, 2};              // Lit when ready for a start

// Discrete in (kSlotDiscreteIn, P1-16ND3) channel map:
//   9   MM exit sensor
//   11  start button
//   12  cancel cook button

// Discrete out (kSlotDiscreteOut, P1-15TD2) channel map:
//   1     GC2 claw
//   2     GC2 lifter
//   3     GC2 capture
//   4     oven hold
//   5     MM capture
//   6     CHOC capture
//   7     GC1 capture
//   8     GC1 pusher solenoid
//   9-11  TBD
//   12    conveyor motor
//   13    GC2 pusher
//   14    CHOC linear actuator
//   15    TBD
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
// Each station's work time comes from its own config: the sum of its actuator
// strokes or moves, the cook time for the oven, or the motor timeout cap for MM.
//
// CALIBRATE all of these against the real belt before running product.

const LinearDispenser::Config kGrahamCracker1 = {
    .capture = {kSlotDiscreteOut, 7},
    .extend = {kSlotDiscreteOut, 8},
    .extendMs = 4000,
    .retractMs = 2500,
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

// Manual heater between chocolate and marshmallow. No outputs: the operator
// releases the tray. The MM stop extends 16 s after the chocolate release, and
// the operator releases the heater within 2 s of that.
const Station::Timing kChocolateHeater = {
    5000,   // Chocolate release -> tray reaches the heater
    11000,  // Heater hold, so MM extends at 16 s
    7000,   // Heater release -> tray fully past the heater
};

const MotorDispenser::Config kMarshmallow = {
    .capture = {kSlotDiscreteOut, 5},
    .motor = {kSlotRelay, 8},
    .exitSensor = {kSlotDiscreteIn, 9},
    .timeoutMs = 2000,
    .transitMs = 5000,
    .clearMs = 7000,
};

// One GC2 cycle, in order.
const GcPusher::Move kGrahamCracker2Moves[] = {
    // lifterUp, clawClosed, pusherOut, ms
    {true, false, true, 8000},    // extend graham cracker
    {false, false, true, 1500},   // lower claw
    {false, true, true, 2000},    // close claw
    {true, true, true, 1500},    // raise claw
    {true, true, false, 8500},    // retract pusher
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

// True runs the oven: the hold solenoid holds the tray, and the heater runs until
// the cook ends. False leaves its outputs and thermistor idle.
const bool kOvenEnabled = true;

const Oven::Config kOven = {
    .enabled = kOvenEnabled,
    .heater = {kSlotRelay, 1},
    .hold = {kSlotDiscreteOut, 4},
    .thermistor = {kSlotThermistor, 1},  // Shown in status
    .cookMs = 26 * 1000,
    .transitMs = 7000,
    .clearMs = 5000,
};

// The oven accepts a new tray once GC2 is free or due to become free within
// this long. 0 requires GC2 fully clear already.
const uint32_t kOvenHandoffSlackMs = 0;

const channelLabel kConveyorMotor = {kSlotDiscreteOut, 12};

// Status LEDs: one strip along the belt, counted from the start of the belt.
// Each station lights its own range. The belt spans the whole strip and is drawn
// first, so the stations cover it and it shows on the LEDs left over.
//
// CALIBRATE the pin, type, count, brightness and ranges against the real strip. LEDs
// past the end of the strip are ignored.

// Arduino pin of the strip's data wire (DIN). It sends 3.3V, so a 5V strip may
// need a level shifter (74AHCT125).
const uint8_t kLedPin = 0;
// Color order and speed. If red shows as green, try NEO_RGB or NEO_BRG.
const neoPixelType kLedType = NEO_GRB + NEO_KHZ800;
const uint16_t kLedCount = 50;
const uint8_t kLedBrightness = 64;  // 0 (dark) to 255 (brightest), for every color

const StatusLeds::Range kBeltLeds = {0, kLedCount};  // First LED, LED count
const StatusLeds::Range kGrahamCracker1Leds = {0, 9};
const StatusLeds::Range kChocolateLeds = {9, 9};
const StatusLeds::Range kChocolateHeaterLeds = {18, 5};
const StatusLeds::Range kMarshmallowLeds = {23, 9};
const StatusLeds::Range kOvenLeds = {32, 9};
const StatusLeds::Range kGrahamCracker2Leds = {41, 9};

const Rgb kRed = {255, 0, 0};
const Rgb kOrange = {255, 128, 0};
const Rgb kYellow = {255, 255, 0};
const Rgb kGreen = {0, 255, 0};
const Rgb kCyan = {0, 255, 255};
const Rgb kBlue = {0, 0, 255};
const Rgb kPink = {255, 0, 128};
const Rgb kPurple = {160, 32, 240};

// Each station's color when the machine is stopped. Pick colors that are easy to tell apart.
const Rgb kBeltIdle = kBlue;
const Rgb kGrahamCracker1Idle = kRed;
const Rgb kChocolateIdle = kOrange;
const Rgb kChocolateHeaterIdle = kPink;
const Rgb kMarshmallowIdle = kYellow;
const Rgb kOvenIdle = kCyan;
const Rgb kGrahamCracker2Idle = kPurple;

// Every station shows:
//
//   idle      solid blue while running, else the station's own color (see above)
//   arriving  blue band, waiting for the tray
//   working   a bar filling from blue to green (or WorkLook::kPulse for a pulse)
//   done      solid green
//   clearing  green band, tray leaving
//
// Bands wrap around the zone with BandMotion::kWrap. BandMotion::kJump starts
// each band again at the first LED. WorkLook::kPulse pulses the station's own
// color.
const StatusLeds::Config kStatusLeds = {
    .runningIdle = kBlue,
    .arriving = kBlue,
    .clearing = kGreen,
    .fillFrom = kBlue,
    .fillTo = kGreen,
    .bandWidth = 3,
    .bandStepMs = 100,
    .band = BandMotion::kWrap,
    .work = WorkLook::kProgress,
};

// Pulse rate for each station, in milliseconds. WorkLook::kPulse also uses the color.
const StatusLeds::Pulse kPulse = {kGreen, 2000};
const StatusLeds::Pulse kOvenPulse = {kRed, 800};

// Watchdog window. HOLD de-energizes every module output and halts the CPU
// until a power cycle, so a hung sketch turns the heater off.
const uint16_t kWatchdogMs = 5000;

}  // namespace config

#endif
