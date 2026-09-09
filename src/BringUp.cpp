#include <Arduino.h>
#include <P1AM.h>

#include "Config.h"
#include "Log.h"
#include "Rig.h"
#include "SerialBoolean.h"

// Bring-up build, for checking wiring before the machine runs. Never starts
// the machine and never starts the watchdog, so nothing needs to pet it.
//
// Powers up read only. Actuators move only after 'actuators' arms them, and
// then only on a press of the start button.

static const uint32_t kReportMs = 5000;

static SerialBoolean actuatorsCommand("actuators", PERSISTENT);

static bool ready = false;
static bool actuatorsArmed = false;
static uint32_t lastReport = 0;

static const char* inputState(channelLabel channel) {
    return P1.readDiscrete(channel) ? "ON" : "off";
}

// One short line per report so a 5s cadence stays readable in the monitor.
static void reportInputs() {
    logLine("%5lus  estop:%s  start:%s  run:%s  oven:%dF", (unsigned long)(millis() / 1000),
            inputState(config::kEStopButton), inputState(config::kStartButton),
            rig::runSwitchOn() ? "ON" : "off",
            (int)P1.readTemperature(config::kOven.thermistor));
}

void setup() {
    ready = rig::begin();
    if (ready) {
        logLine("Bring-up, read only. 'actuators' arms them, start button pulses them.");
    }
}

void loop() {
    rig::pollSerial();

    if (!ready) {
        return;
    }

    bool armedNow = actuatorsCommand.read();
    if (armedNow != actuatorsArmed) {
        actuatorsArmed = armedNow;
        logLine("Actuators %s", actuatorsArmed ? "ARMED" : "disabled, read only");
    }

    if (rig::startEdge()) {
        if (actuatorsArmed) {
            logLine("Pulsing actuators");
            rig::machine().selfTest();
            logLine("Actuator test complete");
        } else {
            logLine("Read only. Type 'actuators' to arm, then press start.");
        }
    }

    if (millis() - lastReport >= kReportMs) {
        lastReport = millis();
        reportInputs();
    }

    digitalWrite(LED_BUILTIN, actuatorsArmed ? HIGH : LOW);
}
