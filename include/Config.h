#ifndef CONFIG_H
#define CONFIG_H

#include <P1AM.h>

#include "Channel.h"
#include "machine/Station.h"
#include "stations/GcPusher.h"
#include "stations/LinearDispenser.h"
#include "stations/Oven.h"

// Every hardware address and dead-reckoning time lives here. Included once,
// from main.cpp. Station order lives in main.cpp, where the stations are built.
namespace config {

struct ModuleSlot {
    const char* name;
    uint8_t slot;  // Slots are 1-indexed
};

// Expected base layout, verified against the base controller at boot. Drop a
// module here if it is not fitted, and set its channels to kAbsent below.
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

// Dead-reckoned travel times, in milliseconds of belt motion.
//
//   transitMs  release upstream -> tray reaches this station's stop
//   clearMs    release -> tray fully past this station, which is the only
//              interlock protecting the station behind it
//
// Work duration is not set here: each station derives it from its own actuator
// sequence, so the two cannot drift apart. CALIBRATE the rest against the real
// belt before running product. The entry station has nothing upstream to
// travel from, hence transitMs 0.
const uint32_t kEntryTransitMs = 0;
const uint32_t kTransitMs = 3000;
const uint32_t kClearMs = 2000;

const Station::Timing kOvenTiming = {kTransitMs, 45000, 2500};

// moveMs order: grab, lift, translate, lower, release, return.
const GcPusher::Config kGrahamCracker1 = {
    {4, 1}, {4, 2}, {4, 3}, {4, 4}, {400, 600, 900, 600, 400, 900}};
const GcPusher::Config kGrahamCracker2 = {
    {4, 10}, {4, 11}, {4, 12}, {4, 13}, {400, 600, 900, 600, 400, 900}};

// capture, extend, then extend/dwell/retract times.
const LinearDispenser::Config kChocolate = {{4, 5}, {4, 6}, 700, 400, 700};
const LinearDispenser::Config kMarshmallow = {{4, 7}, {4, 8}, 700, 400, 700};

// CALIBRATE the setpoint too. 85F is roughly ambient, so as shipped the heater
// never fires and the oven reports ready immediately.
const Oven::Config kOven = {
    {5, 2},  // heater relay
    {4, 9},  // tray hold solenoid
    {2, 1},  // thermistor, or kAbsent to run open-loop
    85.0f,   // setpoint, degF
    1.0f,    // deadband, degF
};

const channelLabel kBeltRelay = {5, 1};

// Watchdog window. HOLD de-energizes every module output and stops the CPU
// until a power cycle, so a hung sketch cannot leave the heater on.
const uint16_t kWatchdogMs = 5000;

}  // namespace config

#endif
