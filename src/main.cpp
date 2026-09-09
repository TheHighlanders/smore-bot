#include <Adafruit_NeoPixel.h>
#include <P1AM.h>

#include "Config.h"
#include "Log.h"
#include "SerialBoolean.h"
#include "machine/Machine.h"
#include "stations/Belt.h"
#include "stations/Dispenser.h"
#include "stations/Oven.h"

static Machine machine;
static Adafruit_NeoPixel pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

static SerialBoolean startCommand("start", EPHEMERAL);
static SerialBoolean eStopCommand("estop", EPHEMERAL);
static SerialBoolean statusCommand("status", EPHEMERAL);

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

// Reads false for an unfitted module, so the same firmware runs on a base
// without the discrete input card.
static bool readInput(channelLabel label) {
    return label.slot != 0 && P1.readDiscrete(label);
}

static bool verifyModules() {
    uint8_t found = P1.printModules();
    if (found != config::kModuleCount) {
        logError("Expected %u modules, base reports %u", (unsigned)config::kModuleCount, found);
        return false;
    }
    for (size_t i = 0; i < config::kModuleCount; i++) {
        const config::ModuleSlot& expected = config::kModules[i];
        moduleProps props = P1.readSlotProps(expected.slot);
        if (strcmp(expected.name, props.moduleName) != 0) {
            logError("Slot %u: expected %s, found %s", expected.slot, expected.name,
                     props.moduleName);
            return false;
        }
    }
    return true;
}

void setup() {
    Serial.begin(115200);
    pinMode(SWITCH_BUILTIN, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
    pixels.begin();
    setRGB(150, 150, 0);

    logUpdate("Smore Bot starting, waiting for base controller");
    while (!P1.init()) {
        ;
    }

    if (!verifyModules()) {
        logError("Module layout does not match Config.h. Fix the base and reboot.");
        return;
    }

    if (config::kOven.thermistor.slot != 0) {
        P1.configureModule(config::kThermistorSetup, config::kOven.thermistor.slot);
    }

    static Dispenser grahamCracker1("GC1", P1, config::kGrahamCracker1,
                                    config::kFirstStationTiming);
    static Dispenser chocolate("CHOC", P1, config::kChocolate, config::kDispenserTiming);
    static Dispenser marshmallow("MM", P1, config::kMarshmallow, config::kDispenserTiming);
    static Oven oven("OVEN", P1, config::kOven, config::kOvenTiming);
    static Dispenser grahamCracker2("GC2", P1, config::kGrahamCracker2, config::kDispenserTiming);
    static Belt belt("BELT", P1, config::kBeltRelay);

    machine.configure({&grahamCracker1, &chocolate, &marshmallow, &oven, &grahamCracker2},
                      {&belt});

    P1.configWD(config::kWatchdogMs, HOLD);
    P1.startWD();

    configOk = true;
    logUpdate("Ready. Flip the run switch, then press start.");
}

void loop() {
    if (Serial.available()) {
        String line = Serial.readStringUntil('\n');
        SerialBoolean::parseInput(line.c_str(), line.length());
    }

    if (!configOk) {
        setRGB(150, 0, 0);
        return;
    }

    P1.petWD();

    bool baseFault = !P1.isBaseActive() || P1.checkConnection() != 0;
    bool eStopPressed = readInput(config::kEStopButton) || eStopCommand.read();

    if (!machine.isEStopped() && (baseFault || eStopPressed)) {
        logError("E-STOP: %s", baseFault ? "base controller fault" : "operator");
        machine.eStop();  // Latched: releasing it needs a power cycle
    }

    machine.run(digitalRead(SWITCH_BUILTIN) == HIGH);

    bool startPressed = readInput(config::kStartButton) || startCommand.read();
    if (startPressed && !startWasPressed && !machine.startCycle()) {
        logError("Cycle refused: %s", machine.isRunning() ? "first station busy" : "machine held");
    }
    startWasPressed = startPressed;

    machine.update();

    if (statusCommand.read()) {
        machine.printStatus();
    }

    if (machine.isEStopped()) {
        setRGB(150, 0, 0);
    } else if (machine.isRunning()) {
        setRGB(150, 150, 0);
    } else {
        setRGB(0, 150, 0);
    }
    digitalWrite(LED_BUILTIN, machine.isRunning() ? HIGH : LOW);
}
