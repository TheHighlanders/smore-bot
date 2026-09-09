#include <Adafruit_NeoPixel.h>
#include <P1AM.h>

#include "Channel.h"
#include "Config.h"
#include "Log.h"
#include "SerialBoolean.h"
#include "machine/Machine.h"
#include "stations/Belt.h"
#include "stations/GcPusher.h"
#include "stations/LinearDispenser.h"
#include "stations/MotorDispenser.h"
#include "stations/Oven.h"

static Machine machine;
static Adafruit_NeoPixel pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

static SerialBoolean startCommand("start", EPHEMERAL);
static SerialBoolean eStopCommand("estop", EPHEMERAL);
static SerialBoolean statusCommand("status", EPHEMERAL);
static SerialBoolean skipCommand("skip", EPHEMERAL);

static bool configOk = false;
static bool startWasPressed = false;

static void setRGB(uint8_t r, uint8_t g, uint8_t b) {
    static uint32_t shown = 0xFFFFFFFFu;
    uint32_t color = pixels.Color(r, g, b);
    if (color == shown) {
        return;
    }
    shown = color;
    pixels.setPixelColor(0, color);
    pixels.show();
}

static void pollSerial() {
    if (!Serial.available()) {
        return;
    }
    String line = Serial.readStringUntil('\n');
    if (!SerialBoolean::parseInput(line.c_str(), line.length())) {
        logError("Unknown command: %s", line.c_str());
    }
}

#ifdef BRINGUP
static void reportInput(const char* label, channelLabel channel) {
    if (!fitted(channel)) {
        logInfo("\t%s: not fitted", label);
    } else {
        logInfo("\t%s: %s", label, readChannel(P1, channel) ? "closed" : "open");
    }
}

// Pulses one actuator at a time and prints every input, so wiring can be
// checked before any machine logic runs.
static void runBringUp() {
    logUpdate("Bring-up: pulsing actuators");
    machine.selfTest();

    logUpdate("Bring-up: inputs");
    reportInput("e-stop button", config::kEStopButton);
    reportInput("start button", config::kStartButton);

    logUpdate("Bring-up complete. Press enter to repeat.");
}
#endif

static bool verifyModules() {
    bool ok = true;
    uint8_t found = P1.printModules();
    if (found != config::kModuleCount) {
        logError("Expected %u modules, base reports %u", (unsigned)config::kModuleCount, found);
        ok = false;
    }
    // Report every mismatch, so a miswired base takes one reboot to diagnose.
    for (size_t i = 0; i < config::kModuleCount; i++) {
        const config::ModuleSlot& expected = config::kModules[i];
        moduleProps props = P1.readSlotProps(expected.slot);
        if (strcmp(expected.name, props.moduleName) != 0) {
            logError("Slot %u: expected %s, found %s", expected.slot, expected.name,
                     props.moduleName);
            ok = false;
        }
    }
    return ok;
}

void setup() {
    Serial.begin(115200);
    Serial.setTimeout(20);  // A partial line must not stall the e-stop scan
    pinMode(SWITCH_BUILTIN, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    pixels.begin();
    setRGB(0, 0, 150);

    logUpdate("Smore Bot starting, waiting for base controller");
    while (!P1.init()) {}

    if (!verifyModules()) {
        logError("Module layout does not match Config.h. Fix the base and reboot.");
        return;
    }

    if (fitted(config::kOven.thermistor)) {
        P1.configureModule(config::kThermistorSetup, config::kOven.thermistor.slot);
    }

    static LinearDispenser grahamCracker1("GC1", P1, config::kGrahamCracker1);
    static LinearDispenser chocolate("CHOC", P1, config::kChocolate);
    static MotorDispenser marshmallow("MM", P1, config::kMarshmallow);
    static Oven oven("OVEN", P1, config::kOven);
    static GcPusher grahamCracker2("GC2", P1, config::kGrahamCracker2);
    static Belt belt("BELT", P1, config::kBeltRelay);

    machine.configure({&grahamCracker1, &chocolate, &marshmallow, &oven, &grahamCracker2},
                      {&belt});

#ifdef BRINGUP
    // Return before the watchdog starts: a bring-up build never runs the
    // machine, so nothing would pet it.
    runBringUp();
    return;
#endif

    P1.configWD(config::kWatchdogMs, HOLD);
    P1.startWD();

    configOk = true;
    logUpdate("Ready. Flip the run switch, then press start.");
}

void loop() {
#ifdef BRINGUP
    setRGB(0, 0, 150);
    if (Serial.available()) {
        Serial.readStringUntil('\n');
        runBringUp();
    }
    return;
#endif

    pollSerial();

    if (!configOk) {
        setRGB(150, 0, 0);
        return;
    }

    // Once e-stopped, stop talking to the base entirely. The watchdog goes
    // unpetted, so it de-energizes every output and holds the CPU until a
    // power cycle. That is the only way out of an e-stop.
    if (machine.isEStopped()) {
        setRGB(150, 0, 0);
        return;
    }

    P1.petWD();

    bool baseFault = !P1.isBaseActive() || P1.checkConnection() != 0;
    bool eStopTyped = eStopCommand.read();  // One-shot: read before any ||
    if (baseFault || readChannel(P1, config::kEStopButton) || eStopTyped) {
        logError("E-STOP: %s", baseFault ? "base controller fault" : "operator");
        machine.eStop();
        setRGB(150, 0, 0);
        return;
    }

    machine.run(digitalRead(SWITCH_BUILTIN) == HIGH);

    // A press while the entry station is occupied is ignored, not queued: the
    // operator can lean on the button and trays still come out one per cycle.
    bool startTyped = startCommand.read();
    bool startPressed = readChannel(P1, config::kStartButton) || startTyped;
    if (startPressed && !startWasPressed && !machine.startCycle()) {
        logInfo("Start ignored: %s", machine.isRunning() ? "entry station busy" : "machine held");
    }
    startWasPressed = startPressed;

    machine.update();

    if (skipCommand.read() && !machine.skipStation()) {
        logError("Nothing to skip");
    }

    if (statusCommand.read()) {
        machine.printStatus();
    }

    // Yellow while the belt moves, green when it is safe to approach.
    if (machine.isRunning()) {
        setRGB(150, 150, 0);
    } else {
        setRGB(0, 150, 0);
    }
    digitalWrite(LED_BUILTIN, machine.isRunning() ? HIGH : LOW);
}
