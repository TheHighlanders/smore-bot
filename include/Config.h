#ifndef CONFIG_H
#define CONFIG_H

#include <P1AM.h>

#include "machine/Station.h"
#include "stations/Dispenser.h"
#include "stations/Oven.h"

// Every hardware address and dead-reckoning time lives here. Included once,
// from main.cpp.
namespace config {

struct ModuleSlot {
    const char* name;
    uint8_t slot;  // Slots are 1-indexed
};

// Expected base layout, verified against the base controller at boot.
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

// Slot 0 marks a channel as not fitted. Inputs read false; outputs are skipped.
const channelLabel kAbsent = {0, 0};

const channelLabel kEStopButton = {1, 9};
const channelLabel kStartButton = {1, 10};

// Dead-reckoned travel times, in milliseconds of belt motion.
//
//   transitMs  release upstream -> tray reaches this station's stop
//   workMs     dispense or cook duration once the tray is here
//   clearMs    release -> tray fully past this station, which is the only
//              interlock protecting the station behind it
//
// CALIBRATE these against the real belt before running product.
const Station::Timing kFirstStationTiming = {0, 1500, 2000};
const Station::Timing kDispenserTiming = {3000, 1500, 2000};
const Station::Timing kOvenTiming = {3000, 45000, 2500};

// Servo pins avoid 0 and 1, which are Serial2 on the P1AM-200.
const Dispenser::Config kGrahamCracker1 = {{4, 1}, 2, 0, 255};
const Dispenser::Config kChocolate = {{4, 2}, 3, 0, 255};
const Dispenser::Config kMarshmallow = {{4, 3}, 4, 0, 255};
const Dispenser::Config kGrahamCracker2 = {{4, 5}, 5, 0, 255};

const Oven::Config kOven = {
    {5, 2},    // heater relay
    {4, 4},    // tray hold solenoid
    {2, 1},    // thermistor, or kAbsent to run open-loop
    85.0f,     // setpoint, degF
    1.0f,      // deadband, degF
};

const channelLabel kBeltRelay = {5, 1};

// Watchdog window. HOLD de-energizes every module output and stops the CPU
// until a power cycle, so a hung sketch cannot leave the heater on.
const uint16_t kWatchdogMs = 5000;

}  // namespace config

#endif
