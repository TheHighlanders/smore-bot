#include <Arduino.h>
#include <P1AM.h>

#include "Config.h"
#include "Log.h"
#include "Rig.h"
#include "SerialBoolean.h"

// Machine build. Runs the line; see BringUp.cpp for the hardware check build.

static SerialBoolean statusCommand("status", EPHEMERAL);

static bool ready = false;

void setup() {
    ready = rig::begin();
    if (!ready) {
        return;
    }

    P1.configWD(config::kWatchdogMs, HOLD);
    P1.startWD();

    logUpdate("Ready. Flip the run switch, then press start.");
}

void loop() {
    rig::pollSerial();

    if (!ready) {
        return;
    }

    Machine& machine = rig::machine();

    // Once e-stopped, stop talking to the base entirely. The watchdog goes
    // unpetted, so it de-energizes every output and holds the CPU until a
    // power cycle. That is the only way out of an e-stop.
    if (machine.isEStopped()) {
        return;
    }

    P1.petWD();

    bool baseFault = !P1.isBaseActive() || P1.checkConnection() != 0;
    if (baseFault || rig::eStopPressed()) {
        logError("E-STOP: %s", baseFault ? "base controller fault" : "operator");
        machine.eStop();
        digitalWrite(LED_BUILTIN, LOW);
        return;
    }

    machine.run(rig::runSwitchOn());

    // A press while the entry station is occupied is ignored, not queued: the
    // operator can lean on the button and trays still come out one per cycle.
    if (rig::startEdge() && !machine.startCycle()) {
        logInfo("Start ignored: %s", machine.isRunning() ? "entry station busy" : "machine held");
    }

    machine.update();

    if (statusCommand.read()) {
        machine.printStatus();
    }

    digitalWrite(LED_BUILTIN, machine.isRunning() ? HIGH : LOW);
}
