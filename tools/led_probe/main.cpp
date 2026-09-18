#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include <string>

#include "Log.h"
#include "Probe.h"

// Finds the speed and color order of a 3-wire LED strip. See README.md.

const uint8_t kDataPin = 4;
const uint16_t kMaxLeds = 1000;  // Data goes to this many LEDs, so a long strip goes dark
const uint8_t kLevel = 64;       // Brightness, 0 to 255

// Four bytes per LED fit every strip.
const size_t kFrameBytes = kMaxLeds * 4;
Adafruit_NeoPixel strip(kMaxLeds, kDataPin, NEO_RGBW + NEO_KHZ800);

const char* const kThreeColors =
    "The first 3 LEDs should light in 3 different colors. The rest stay dark.\n"
    "LED 1 is nearest the wires. A group of 3 LEDs that light together counts as one.\n"
    "Type the colors, LED 1 first: R = red, G = green, B = blue, W = white.\n"
    "Example: LED 1 red, LED 2 blue, LED 3 green. Type RBG.\n"
    "Type X if the lights flicker, show random colors, or stay dark.";
const char* const kFourColors =
    "The first 4 LEDs should light in 4 different colors.\n"
    "Type all four, LED 1 first. Example: RBGW\n"
    "Type X if the lights look wrong.";

std::string ask(const char* prompt) {
    while (Serial.available()) {
        Serial.read();  // Ignore keys pressed early
    }
    Serial.println(prompt);
    while (!Serial.available()) {}
    String line = Serial.readStringUntil('\n');
    line.trim();
    line.toUpperCase();
    return line.c_str();
}

// Lights n LEDs in n colors at this speed. Reads what the person types.
probe::Reading askColors(int khz, size_t n, std::string* typed) {
    probe::colorFrame(strip.getPixels(), kFrameBytes, n, kLevel);
    strip.updateType(NEO_RGBW + (khz == 400 ? NEO_KHZ400 : NEO_KHZ800));
    strip.show();
    for (;;) {
        *typed = ask(n == 4 ? kFourColors : kThreeColors);
        probe::Reading reading = probe::readColors(*typed, n);
        if (reading != probe::Reading::Unclear) {
            return reading;
        }
        Serial.println("Type letters like the example, or X.");
    }
}

// True when the strip answers at this speed.
bool findOrder(int khz, std::string* order) {
    probe::Reading reading = askColors(khz, 3, order);
    if (reading == probe::Reading::Same) {
        Serial.println("All 3 LEDs were the same color, so the strip may have a white light.");
        reading = askColors(khz, 4, order);
    }
    return reading == probe::Reading::Order;
}

void printIntro() {
    Serial.println("LED strip probe");
    Serial.println("Finds the speed and color order of a 3-wire LED strip.");
    Serial.println("");
    Serial.println("Before you start:");
    Serial.println("  1. Turn off the robot's 24V power. The probe runs from the USB cable.");
    Serial.println("  2. Join the power supply's ground to the controller's GND.");
    logLine("  3. Connect the strip's data wire (DIN) to pin %u.", kDataPin);
    Serial.println("  4. Power the strip with the voltage printed on it.");
    Serial.println("     If no voltage is printed, use 5V.");
}

void printHelp() {
    Serial.println("No luck at either speed. Check these, then try again:");
    Serial.println("  - The strip has power, and its ground joins the controller's GND.");
    Serial.println("  - The data wire is on DIN, where the arrows on the strip start.");
    Serial.println("  - The voltage matches the strip. Cut marks every 3 LEDs mean 12V.");
    Serial.println("  - A 3.3V to 5V level shifter (74AHCT125) is on the data wire.");
    Serial.println("  - The strip has at least 4 LEDs.");
    Serial.println("  - The strip has a data wire (DIN).");
}

void printResults(int khz, const std::string& order) {
    Serial.println("Results");
    logLine("  Speed:        %d kHz", khz);
    logLine("  Color order:  %s", order.c_str());
    logLine("  Setting:      NEO_%s + NEO_KHZ%d", order.c_str(), khz);
    Serial.println("Use the voltage printed on the strip.");
}

void setup() {
    Serial.begin(115200);
    strip.begin();
    while (!Serial) {}
}

void loop() {
    printIntro();
    ask("Press Enter when the strip has power.");

    int khz = 800;
    std::string order;
    if (!findOrder(khz, &order)) {
        Serial.println("Trying the slower speed.");
        khz = 400;
        if (!findOrder(khz, &order)) {
            printHelp();
            return;
        }
    }

    printResults(khz, order);
    ask("Press Enter to run the probe again.");
}
