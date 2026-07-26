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

#include <P1AM.h>
#include "machine/Machine.h"
#include "stations/ExampleStation.h"

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

  ExampleStation example("ExampleStation");

  std::vector<Station*> stations{&example};

  Machine smoreBot(stations);
}

void loop() {
  P1.writeDiscrete(HIGH, 1, 2);   // Slot 1, Channel 2 → ON
  Serial.println("Output ON");
  delay(1000);

  P1.writeDiscrete(LOW, 1, 2);    // Slot 1, Channel 2 → OFF
  Serial.println("Output OFF");
  delay(1000);
}