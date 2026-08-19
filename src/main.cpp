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

    setRGB(150, 150, 0);

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
    for(const auto& module : modules){
        moduleProps props = P1.readSlotProps(module.second);

        if(strcmp(module.first.c_str(), props.moduleName) != 0){
            Serial.printf("Error: Configuration incorrect. Slot %d\r\nExpected: %s, Found: %s\r\n", module.second, module.first, props.moduleName);
            moduleNameError = true;
        }
    }

    if(moduleNameError){
        Serial.println("Resolve Module Configuration Ordering Errors");
        return;
    }

    Serial.println("Modules List Confirmed Correct");

    // Station Creation
    Dispenser* gc1= new Dispenser("GC1", P1, gc1Config);
    Dispenser* choc = new Dispenser("CHOC", P1, chocConfig);
    Dispenser* mm = new Dispenser("MM", P1, mmConfig);
    Oven* oven = new Oven("Oven", P1, ovenConfig, &smoreBot);
    Dispenser* gc2 = new Dispenser("GC2", P1, gc2Config);

    Belt* belt = new Belt("BELT", P1, beltConfig);

    Serial.println("Stations Instantiated");

    std::vector<Station*> stations{gc1, choc, mm, oven, gc2};
    std::vector<Station*> contStations{belt};

    // Machine Setup
    smoreBot = Machine(stations, contStations);

    for(auto station : stations){
        Serial.printf("Station: %s\r\n", station->name().c_str());
    }
    for(auto station : contStations){
        Serial.printf("Continuous Station: %s\r\n", station->name().c_str());
    }

    Serial.println("Configuration Complete, Machine Ready");
    setRGB(0,150,0);
}

void loop() {
    smoreBot.tickTimers();
    smoreBot.update();

    // Connection Checking
    int fault = P1.checkConnection();
    if (!P1.isBaseActive() || fault) {
        Serial.printf("Machine E-Stopping, Fault at %d\r\n", fault);
        smoreBot.eStop();
        setRGB(255,0,0);
    }

    // Poll Buttons
    // Run switch
    if (digitalRead(SWITCH_BUILTIN) && !smoreBot.isEmergencyStopped()) {
        smoreBot.resume();
    } else {
        smoreBot.stop();
    }

    // EStop
    if ((P1.readDiscrete(eStop) || eStopSerial.read()) && !smoreBot.isEmergencyStopped()) {
        Serial.println("E-STOPPED");
        smoreBot.eStop();
        setRGB(255,0,0);
        // Machine will need to be power cycled to release E-Stop

    }

    // Start Button
    if (P1.readDiscrete(startButton) || startSerial.read()) {
        logUpdate("Starting Cycle");
        if(!smoreBot.startCycle()){
            logError("Machine Refused Cycle Start");
            logError("Error: %s", (smoreBot.isRunning() ? "First Station Refused" : "Machine is not running"));
        }
    }

    if (smoreBot.isRunning()){
        setRGB(150, 150, 0);
        digitalWrite(LED_BUILTIN, HIGH);
    } else {
        if(!smoreBot.isEmergencyStopped()){
            setRGB(0,255,0);
        }
        digitalWrite(LED_BUILTIN, LOW);
    }

    if(Serial.available()){
        String str = Serial.readStringUntil('\n');
        Serial.printf("Received: %s\r\n", str.c_str());
        // Update our Serial Booleans
        SerialBoolean::parseInput(str.c_str(), str.length());
    }

    if(statusSerial.read()){
        smoreBot.printStatus();
    }
}

void setRGB(int r, int g, int b){
    pixels.setPixelColor(0, pixels.Color(r, g, b)); // Set RGB LED to green (R, G, B)
    pixels.show(); // Update RGB LED        
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
    pinMode(LED_BUILTIN, OUTPUT); // Configure inbuilt LED (Non-RGB)
    pixels.begin();

    eStop = {modules.at("P1-16ND3"), 9};
    startButton = {modules.at("P1-16ND3"),10};
}

void configureStations() {
    gc1Config = {
        {modules.at("P1-15TD2"), 1},  // capture
        0,                            // dispense GPIO 0
        {modules.at("P1-16ND3"), 1},  // traySense
        0,                            // active
        0,                            // inactive
    };

    chocConfig = {
        {modules.at("P1-15TD2"), 2},  // capture
        1,                            // dispense GPIO 1
        {modules.at("P1-16ND3"), 2},  // traySense
        0,                            // active
        0,                            // inactive
    };

    mmConfig = {
        {modules.at("P1-15TD2"), 3},  // capture
        2,                            // dispense GPIO 2
        {modules.at("P1-16ND3"), 3},  // traySense
        0,                            // active
        0,                            // inactive
    };
    ovenConfig = {
        {modules.at("P1-08TRS"), 2},  // relay
        {modules.at("P1-16ND3"), 4},  // entry
        {modules.at("P1-16ND3"), 5},  // exit
        {modules.at("P1-15TD2"), 4},  // capture
        {modules.at("P1-04NTC"), 1},  // thermocouple
        100,                            // setpoint;
        5,                            // deadzone
    };

    gc2Config = {
        {modules.at("P1-15TD2"), 5},  // capture
        3,                            // dispense GPIO 3
        {modules.at("P1-16ND3"), 6},  // traySense
        0,                            // active
        0,                            // inactive
    };

    beltConfig = {
        {modules.at("P1-08TRS"), 1}   // relay
    };
}