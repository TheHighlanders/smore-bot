#include <Arduino.h>
#include <P1AM.h>

#include "Config.h"
#include "Log.h"
#include "Rig.h"
#include "SerialBoolean.h"

// Machine build. Runs the line; see BringUp.cpp for the hardware check build.

static SerialBoolean statusCommand("status", EPHEMERAL);
static SerialBoolean startCommand("start", EPHEMERAL);

static bool ready = false;

void setup() {
    ready = rig::begin();
    if (!ready) {
        return;
    }

    P1.configWD(config::kWatchdogMs, HOLD);
    P1.startWD();

    logLine("Ready. Flip the run switch, then press start.");
}

void loop() {
    rig::pollSerial();

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
    // Switching off resets every station; the operator clears the belt before
    // switching back on.
    machine.run(rig::runSwitchOn());

    // A press starts a cycle when the entry station is free, so a held button
    // yields one tray per cycle.
    if ((startCommand.read() || rig::startEdge()) && !machine.startCycle()) {
        logLine("Start ignored: %s", machine.isRunning() ? "entry station busy" : "machine stopped");
    }

    machine.update();

    if (statusCommand.read()) {
        machine.printStatus();
    }

    digitalWrite(LED_BUILTIN, machine.isRunning() ? HIGH : LOW);
}
