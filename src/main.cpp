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

    Serial.println("Smore Bot Online");
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
    moduleNames[0] = "";
    for (const auto& module : modules) {
        if(module.second > numModules){
            Serial.println("Module Configured in a slot larger than the number of configured modules");
            Serial.println("\tLikely Caused by missing a module in configureModules()");
            Serial.println("\tResolve and Reboot");
            Serial.printf("Number of modules configured: %d, Module listed in position: %d\r\n", numModules, module.second);
            return;
        }
        moduleNames[module.second - 1] =  // Subtract one to account for 1 indexing of slot numbers
            module.first.c_str();  // Convert to C Strings
    }

    Serial.println("Checking Module Configuration");

    Serial.println("Configured Modules:");
    for (auto name : moduleNames) {
         Serial.println(name);
    }

    int found = P1.printModules();
    Serial.printf("Expected: %d, Actual: %d\r\n", numModules, found);
    if(found != numModules){
        Serial.println("Discrepancy Detected");
    }

    Serial.println("Checking Correct Ordering");

    bool moduleNameError = false;
    for(const auto& [name, slot] : modules){
        moduleProps props = P1.readSlotProps(slot);

        if(strcmp(name.c_str(), props.moduleName) != 0){
            Serial.printf("Error: Configuration incorrect. Slot %d\r\nExpected: %s, Found: %s\r\n", slot, name, props.moduleName);
            moduleNameError = true;
        }
    }

    if(moduleNameError){
        Serial.println("Resolve Module Configuration Ordering Errors");
        return;
    }

    Serial.println("Modules List Confirmed Correct");

    // Station Creation
    Dispenser gc1("GC1", P1, gc1Config);
    Dispenser choc("CHOC", P1, chocConfig);
    Dispenser mm("MM", P1, mmConfig);
    Oven oven("Oven", P1, ovenConfig);
    Dispenser gc2("GC2", P1, gc2Config);

    Serial.println("Stations Instantiated");

    std::vector<Station*> stations{&gc1, &choc, &mm, &oven, &gc2};

    // Machine Setup
    smoreBot = Machine(stations);

    Serial.println("Configuration Complete, Machine Ready");
}

void loop() {
    smoreBot.update();

    // Connection Checking
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
    // Slot numbers are NOT zero indexed (ie slot 1 is the first connected module)
    modules.emplace("P1-16ND3", 1);  // Digital Input
    modules.emplace("P1-04NTC", 2);  // Thermistor
    modules.emplace("P1-04AD-2", 3); // Analog Input
    modules.emplace("P1-15TD2", 4);  // Digital Output
    modules.emplace("P1-08TRS", 5);  // Relay
}

void configureModules() {
    // Configure Thermistor
    // https://facts-engineering.github.io/modules/P1-04NTC/P1-04NTC.html
    // High Side Burnout degF, 10k-CP (Type 3), All channels enabled
    const char P1_04NTC_CONFIG[] = {0x40, 0x03, 0x60, 0x07,
                                    0x20, 0x02, 0x80, 0x00};
    P1.configureModule(P1_04NTC_CONFIG, modules.at("P1-04NTC"));

    pinMode(SWITCH_BUILTIN, INPUT);  // Configure inbuilt switch

    eStop = {modules.at("P1-16ND3"), 0};
    startButton = {modules.at("P1-16ND3"),0};
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