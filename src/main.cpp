#include <Arduino.h>
#include <P1AM.h>

#include "Config.h"
#include "Log.h"
#include "Rig.h"
#include "SerialBoolean.h"

// Normal mode runs the line. Type 'debug' to check the wiring instead; type it
// again to go back. See README.

static const uint32_t kModeReportMs = 15000;
static const uint32_t kInputReportMs = 5000;
static const uint32_t kStartDelayMs = 1000;

static SerialBoolean statusCommand("status", EPHEMERAL);
static SerialBoolean startCommand("start", EPHEMERAL);
static SerialBoolean cancelCookCommand("cancel cook", EPHEMERAL);

static bool ready = false;
static bool debugMode = false;
static uint32_t lastModeReport = 0;
static uint32_t lastInputReport = 0;
static bool startPending = false;
static uint32_t startRequestedAt = 0;

static const char* inputState(channelLabel channel) {
    return P1.readDiscrete(channel) ? "ON" : "off";
}

static void reportMode() {
    logLine(debugMode ? "Debug mode" : "Smore mode");
}

// One short line so a 5s cadence stays readable in the monitor.
static void reportInputs() {
    char oven[12] = "disabled";
    if (config::kOvenEnabled) {
        snprintf(oven, sizeof(oven), "%dF", (int)P1.readTemperature(config::kOven.thermistor));
    }
    logLine("start:%s  cancelCook:%s  mmExit:%s  run:%s  oven:%s", inputState(config::kStartButton),
            inputState(config::kCancelCookButton), inputState(config::kMarshmallow.exitSensor),
            rig::runSwitchOn() ? "ON" : "off", oven);
}

static void handleLine(const String& line) {
    if (line == "debug") {
        debugMode = !debugMode;
        // selfTest holds actuators with delay() past the watchdog window, so
        // debug mode stops the watchdog the same as the old bring-up build did.
        if (debugMode) {
            P1.stopWD();
        } else {
            P1.startWD();
        }
        reportMode();
        lastModeReport = millis();
        return;
    }
    if (!debugMode) {
        if (!SerialBoolean::parseInput(line.c_str(), line.length())) {
            logLine("Unknown command: %s", line.c_str());
        }
        return;
    }
    // Typing 'debug' is already an explicit opt-in, so a station name runs
    // its selfTest right away.
    if (!rig::machine().selfTestNamed(line.c_str())) {
        logLine("Unknown station: %s", line.c_str());
    }
}

void setup() {
    ready = rig::begin();
    if (!ready) {
        return;
    }

    P1.configWD(config::kWatchdogMs, HOLD);
    P1.startWD();

    logLine("Ready. Flip the run switch, then press start. Type 'debug' to check wiring.");
}

void loop() {
    String line;
    if (rig::readLine(line)) {
        handleLine(line);
    }

    if (!ready) {
        return;
    }

    // Base controller lost: skip the pet so the HOLD watchdog de-energizes
    // every output.
    if (!P1.isBaseActive() || P1.checkConnection() != 0) {
        logLine("Base controller fault: not petting the watchdog");
        digitalWrite(LED_BUILTIN, LOW);
        return;
    }

    P1.petWD();

    Machine& machine = rig::machine();
    // Debug mode holds the machine stopped, the same as the run switch being
    // off, so it's safe to check wiring or run a selfTest.
    machine.run(!debugMode && rig::runSwitchOn());

    // A press starts a cycle when the entry station is free, so a held button
    // yields one tray per cycle.
    if (startCommand.read() || rig::startEdge()) {
        startPending = true;
        startRequestedAt = millis();
    }

    // Waits kStartDelayMs after the press so the operator has a moment clear
    // of the line before the sequence begins.
    if (startPending && millis() - startRequestedAt >= kStartDelayMs) {
        startPending = false;
        if (!machine.startCycle()) {
            logLine("Start ignored: %s", machine.isRunning() ? "entry station busy" : "machine stopped");
        }
    }

    machine.update();

    if (statusCommand.read()) {
        machine.printStatus();
    }

    if (cancelCookCommand.read() || rig::cancelCookEdge()) {
        if (rig::oven().cancelCook()) {
            logLine("Oven: cook canceled");
        } else {
            logLine("Cancel cook ignored: oven not cooking");
        }
    }

    if (debugMode && millis() - lastInputReport >= kInputReportMs) {
        lastInputReport = millis();
        reportInputs();
    }

    if (millis() - lastModeReport >= kModeReportMs) {
        lastModeReport = millis();
        reportMode();
    }

    digitalWrite(LED_BUILTIN, (debugMode || machine.isRunning()) ? HIGH : LOW);
}
