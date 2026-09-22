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
Oven* g_oven = nullptr;
bool g_startWasPressed = false;
bool g_cancelCookWasPressed = false;

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
Oven& oven() { return *g_oven; }

bool begin() {
    Serial.begin(115200);
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
    static LinearDispenser chocolate("CH", P1, config::kChocolate);
    static MotorDispenser marshmallow("MM", P1, config::kMarshmallow);
    static GcPusher grahamCracker2("GC2", P1, config::kGrahamCracker2);
    static Oven oven("OVEN", P1, config::kOven, [](uint32_t clock) {
        return grahamCracker2.freeWithin(clock, config::kOvenHandoffSlackMs);
    });
    static Belt belt("BELT", P1, config::kConveyorMotor);

    g_oven = &oven;
    g_machine.configure({&grahamCracker1, &chocolate, &marshmallow, &oven, &grahamCracker2},
                        {&belt});
    return true;
}

// Consumes only bytes already buffered, so a partial command returns at once
// and the caller's loop keeps its timing.
bool readLine(String& line) {
    static String buffer;
    while (Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n') {
            line = buffer;
            line.trim();
            line.toLowerCase();  // Commands and station names match in any case
            buffer = "";
            return true;
        }
        if (buffer.length() < 64) {
            buffer += c;
        }
    }
    return false;
}

bool runSwitchOn() { return digitalRead(SWITCH_BUILTIN) == HIGH; }

bool startEdge() {
    bool pressed = P1.readDiscrete(config::kStartButton);
    bool edge = pressed && !g_startWasPressed;
    g_startWasPressed = pressed;
    return edge;
}

bool cancelCookEdge() {
    bool pressed = P1.readDiscrete(config::kCancelCookButton);
    bool edge = pressed && !g_cancelCookWasPressed;
    g_cancelCookWasPressed = pressed;
    return edge;
}

}  // namespace rig
