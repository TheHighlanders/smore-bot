/*
 * P1AM Blink Example
 *
 * Initializes the P1AM Base Controller, then toggles discrete output
 * channel 2 on slot 1 at 1-second intervals.
 *
 * Compatible with:
 *   - P1AM-100  (ATSAMD21G18A)
 *   - P1AM-200  (ATSAMD51P20A)
 *
 * Wiring: Connect a P1000-series digital output module in slot 1.
 *
 * Library: https://github.com/facts-engineering/P1AM
 */

#include "hardware/P1AMLogged.h"
#include "machine/Machine.h"
#include "stations/ExampleStation.h"
#include "stations/Dispenser.h"
#include "stations/Oven.h"
#include "main.h"

void setup() {
  Serial.begin(115200);

  Serial.println("P1AM Blink Example");
  Serial.println("Waiting for Base Controller...");

  // P1.init() returns true once all modules have finished initializing.
  // It will block here until modules are ready.
  while (!P1.init()) {
    ;
  }

  Serial.println("Base Controller ready.");

  // Configurations
  Dispenser::DispenserHWConfig gc1Config = {
    {0,0}, // capture
    {0,0}, // dispense
    {0,0}, // traySense
    0, // active
    0, // inactive
  };

    Dispenser::DispenserHWConfig chocConfig = {
    {0,0}, // capture
    {0,0}, // dispense
    {0,0}, // traySense
    0, // active
    0, // inactive
  };

    Dispenser::DispenserHWConfig mmConfig = {
    {0,0}, // capture
    {0,0}, // dispense
    {0,0}, // traySense
    0, // active
    0, // inactive
  };

  Oven::OvenHWConfig ovenConfig = {
    {0,0}, // relay
    {0,0}, // entry
    {0,0}, // exit
    {0,0}, // thermocouple
    0, // setpoint;
    0, // deadzone
  };

    Dispenser::DispenserHWConfig gc2Config = {
    {0,0}, // capture
    {0,0}, // dispense
    {0,0}, // traySense
    0, // active
    0, // inactive
  };

  // Station Creation
  Dispenser gc1("GC1", P1, gc1Config);
  Dispenser choc("CHOC", P1, chocConfig);
  Dispenser mm("MM", P1, mmConfig);
  Oven      oven("Oven", P1, ovenConfig);
  Dispenser gc2("GC2", P1, gc2Config);

  std::vector<Station*> stations{&gc1, &choc, &mm, &oven, &gc2};

  // Machine Setup
  smoreBot = Machine(stations);
}

void loop() {
    smoreBot.update();

    // Poll Buttons (TODO, add checks + global config)
    // Start Button
    if (false) {
        smoreBot.startCycle();
    }

    // EStop
    if(false) {
        smoreBot.eStop();
    }
}