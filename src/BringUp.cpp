#include <Arduino.h>
#include <P1AM.h>

#include "Config.h"
#include "Log.h"
#include "Rig.h"

// Bring-up build, for checking wiring before the machine runs. Never starts
// the machine and never starts the watchdog, so nothing needs to pet it.
//
// Type 'actuators' to arm, then a station's name to run its sequence.
// Buttons and other inputs are only ever reported here, never acted on.

static const uint32_t kReportMs = 5000;

static bool ready = false;
static bool armed = false;
static uint32_t lastReport = 0;

static const char* inputState(channelLabel channel) {
    return P1.readDiscrete(channel) ? "ON" : "off";
}

// One short line per report so a 5s cadence stays readable in the monitor.
static void reportInputs() {
    char oven[12] = "disabled";
    if (config::kOvenEnabled) {
        snprintf(oven, sizeof(oven), "%dF", (int)P1.readTemperature(config::kOven.thermistor));
    }
    logLine("%5lus  start:%s  mmExit:%s  run:%s  oven:%s", (unsigned long)(millis() / 1000),
            inputState(config::kStartButton), inputState(config::kMarshmallow.exitSensor),
            rig::runSwitchOn() ? "ON" : "off", oven);
}

static void handleLine(const String& line) {
    if (line == "arm") {
        armed = !armed;
        logLine("Actuators %s", armed ? "ARMED" : "disabled, read only");
        return;
    }
    if (!armed) {
        logLine("Read only. Type 'arm' to arm, then a station name.");
        return;
    }
    if (!rig::machine().selfTestNamed(line.c_str())) {
        logLine("Unknown station: %s", line.c_str());
    }
}

void setup() {
    ready = rig::begin();
    if (ready) {
        logLine("Bring-up, read only. 'arm' arms them, then type a station name.");
    }
}

void loop() {
    String line;
    if (rig::readLine(line)) {
        handleLine(line);
    }

    if (!ready) {
        return;
    }

    if (millis() - lastReport >= kReportMs) {
        lastReport = millis();
        reportInputs();
    }

    digitalWrite(LED_BUILTIN, armed ? HIGH : LOW);
}
