#ifndef CONFIG_H
#define CONFIG_H

#include <P1AM.h>

#include "machine/Station.h"
#include "stations/GcPusher.h"
#include "stations/LinearDispenser.h"
#include "stations/MotorDispenser.h"
#include "stations/Oven.h"

// Every hardware address and dead-reckoning time lives here. Included once,
// from main.cpp. Station order lives in main.cpp, where the stations are built.
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
// sequence, so timing cannot drift out of step with the moves performed. The
// entry station has nothing upstream to travel from, hence transitMs 0.
//
// CALIBRATE all of these against the real belt before running product.

// capture, extend, then extend/dwell/retract, transit, clear.
const LinearDispenser::Config kGrahamCracker1 = {{4, 1}, {4, 2}, 700, 400, 700, 0, 2000};
const LinearDispenser::Config kChocolate = {{4, 3}, {4, 4}, 700, 400, 700, 3000, 2000};

// TBD hardware: a hobby motor on a discrete output. capture, motor, then
// run/settle, transit, clear.
const MotorDispenser::Config kMarshmallow = {{4, 5}, {4, 6}, 1200, 600, 3000, 2000};

// capture, gripper, lift, translate, then moveMs in the order
// grab, lift, translate, lower, release, return, then transit, clear.
const GcPusher::Config kGrahamCracker2 = {
    {4, 8}, {4, 9}, {4, 10}, {4, 11}, {400, 600, 900, 600, 400, 900}, 3000, 2000};

// CALIBRATE the setpoint too. 85F is roughly ambient, so as shipped the heater
// never fires and the oven reports ready immediately.
const Oven::Config kOven = {
    {5, 2},  // heater relay
    {4, 7},  // tray hold solenoid
    {2, 1},  // thermistor
    85.0f,   // setpoint, degF
    1.0f,    // deadband, degF
    45000,   // cook time
    3000,    // transit
    2500,    // clear
};

const channelLabel kBeltRelay = {5, 1};

// Watchdog window. HOLD de-energizes every module output and stops the CPU
// until a power cycle, so a hung sketch cannot leave the heater on.
const uint16_t kWatchdogMs = 5000;

}  // namespace config

#endif
