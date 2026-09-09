#include "Rig.h"

#include <Arduino.h>
#include <P1AM.h>
#include <string.h>

#include "Config.h"
#include "Log.h"
#include "SerialBoolean.h"
#include "stations/Belt.h"
#include "stations/GcPusher.h"
#include "stations/LinearDispenser.h"
#include "stations/MotorDispenser.h"
#include "stations/Oven.h"

namespace rig {
namespace {

Machine g_machine;
bool g_startWasPressed = false;

bool verifyModules() {
    bool ok = true;
    uint8_t found = P1.printModules();
    if (found != config::kModuleCount) {
        logLine("Expected %u modules, base reports %u", (unsigned)config::kModuleCount, found);
        ok = false;
    }
    // Report every mismatch, so a miswired base takes one reboot to diagnose.
    for (size_t i = 0; i < config::kModuleCount; i++) {
        const config::ModuleSlot& expected = config::kModules[i];
        moduleProps props = P1.readSlotProps(expected.slot);
        if (strcmp(expected.name, props.moduleName) != 0) {
            logLine("Slot %u: expected %s, found %s", expected.slot, expected.name,
                     props.moduleName);
            ok = false;
        }
    }
    return ok;
}

}  // namespace

Machine& machine() { return g_machine; }

bool begin() {
    Serial.begin(115200);
    Serial.setTimeout(20);  // A partial line must not stall the e-stop scan
    pinMode(SWITCH_BUILTIN, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);

    logLine("Smore Bot starting, waiting for base controller");
    logLine("No response? Check the external 24V supply is on.");
    while (!P1.init()) {}

    if (!verifyModules()) {
        logLine("Module layout does not match Config.h. Fix the base and reboot.");
        return false;
    }

    if (config::kOvenEnabled) {
        P1.configureModule(config::kThermistorSetup, config::kOven.thermistor.slot);
    }

    static LinearDispenser grahamCracker1("GC1", P1, config::kGrahamCracker1);
    static LinearDispenser chocolate("CHOC", P1, config::kChocolate);
    static MotorDispenser marshmallow("MM", P1, config::kMarshmallow);
    static Oven oven("OVEN", P1, config::kOven);
    static GcPusher grahamCracker2("GC2", P1, config::kGrahamCracker2);
    static Belt belt("BELT", P1, config::kBeltRelay);

    g_machine.configure({&grahamCracker1, &chocolate, &marshmallow, &oven, &grahamCracker2},
                        {&belt});
    return true;
}

void pollSerial() {
    if (!Serial.available()) {
        return;
    }
    String line = Serial.readStringUntil('\n');
    if (!SerialBoolean::parseInput(line.c_str(), line.length())) {
        logLine("Unknown command: %s", line.c_str());
    }
}

bool runSwitchOn() { return digitalRead(SWITCH_BUILTIN) == HIGH; }

bool eStopPressed() { return P1.readDiscrete(config::kEStopButton); }

bool startEdge() {
    bool pressed = P1.readDiscrete(config::kStartButton);
    bool edge = pressed && !g_startWasPressed;
    g_startWasPressed = pressed;
    return edge;
}

}  // namespace rig
