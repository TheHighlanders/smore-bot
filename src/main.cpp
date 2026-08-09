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

#include "main.h"

#include "P1AM.h"
#include "machine/Machine.h"

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
    configureMachine();
    configureModules();
    configureStations();

    int numModules = modules.size();
    const char* moduleNames[numModules];
    for (auto module : modules) {
        if(module.second >= numModules){
            Serial.println("Module Configured in a slot larger than the number of configured modules");
            Serial.println("\tLikely Caused by missing a module in configureModules()");
            Serial.println("\tResolve and Reboot");
            return;
        }
        moduleNames[module.second] =
            module.first.c_str();  // Convert to C Strings
    }

    // Ensure Machine is intact
    int machineOK = P1.rollCall(moduleNames, modules.size());
    if (!machineOK) {
        Serial.println("Machine Configuration Errors. Modules Connected:");
        int found = P1.printModules();
        Serial.println("Configuration Expected:");
        for (auto name : moduleNames) {
            Serial.println(name);
        }
        Serial.printf("Expected: %d, Actual: %d\r\n", numModules, found);
    }

    // Station Creation
    Dispenser gc1("GC1", P1, gc1Config);
    Dispenser choc("CHOC", P1, chocConfig);
    Dispenser mm("MM", P1, mmConfig);
    Oven oven("Oven", P1, ovenConfig);
    Dispenser gc2("GC2", P1, gc2Config);

    std::vector<Station*> stations{&gc1, &choc, &mm, &oven, &gc2};

    // Machine Setup
    smoreBot = Machine(stations);
}

void loop() {
    smoreBot.update();

    // TODO: Connection checking
    int fault = P1.checkConnection();
    if (!P1.isBaseActive() || fault) {
        Serial.printf("Machine E-Stopping, Fault at %d\r\n", fault);
        smoreBot.eStop();
    }

    // Poll Buttons
    // Run switch
    if (digitalRead(SWITCH_BUILTIN)) {
        smoreBot.resume();
    } else {
        smoreBot.stop();
    }

    // EStop
    if (P1.readDiscrete(eStop)) {
        smoreBot.eStop();
        // Machine will need to be power cycled to release E-Stop
    }

    // Start Button
    if (P1.readDiscrete(startButton)) {
        smoreBot.startCycle();
    }
}

void configureMachine() {
    // Add modules in format (Name, Slot) to the modules list.
    modules.emplace("P1-15TD2", 0);  // Digital Output
    modules.emplace("P1-08TRS", 0);  // Relay
    modules.emplace("P1-04NTC", 0);  // Thermistor
}

void configureModules() {
    // Configure Thermistor
    // https://facts-engineering.github.io/modules/P1-04NTC/P1-04NTC.html
    // High Side Burnout degF, 10k-CP (Type 3), All channels enabled
    const char P1_04NTC_CONFIG[] = {0x40, 0x03, 0x60, 0x07,
                                    0x20, 0x02, 0x80, 0x00};
    P1.configureModule(P1_04NTC_CONFIG, modules.at("P1-04NTC"));

    pinMode(SWITCH_BUILTIN, INPUT);  // Configure inbuilt switch
}

void configureStations() {
    gc1Config = {
        {modules.at("P1-15TD2"), 0},  // capture
        PIN_A0,                       // dispense
        {0, 0},                       // traySense
        0,                            // active
        0,                            // inactive
    };

    chocConfig = {
        {modules.at("P1-15TD2"), 0},  // capture
        PIN_A0,                       // dispense
        {0, 0},                       // traySense
        0,                            // active
        0,                            // inactive
    };

    mmConfig = {
        {modules.at("P1-15TD2"), 0},  // capture
        PIN_A0,                       // dispense
        {0, 0},                       // traySense
        0,                            // active
        0,                            // inactive
    };
    ovenConfig = {
        {modules.at("P1-15TD2"), 0},  // relay
        {0, 0},                       // entry
        {0, 0},                       // exit
        {0, 0},
        {modules.at("P1-04NTC"), 0},  // thermocouple
        0,                            // setpoint;
        0,                            // deadzone
    };

    gc2Config = {
        {modules.at("P1-15TD2"), 0},  // capture
        PIN_A0,                       // dispense
        {0, 0},                       // traySense
        0,                            // active
        0,                            // inactive
    };
}