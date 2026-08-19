#ifndef MAIN_h
#define MAIN_h

#include "machine/Machine.h"
#include "stations/Dispenser.h"
#include "stations/Oven.h"
#include "stations/Belt.h"
#include "SerialBoolean.h"

#include <P1AM.h>
#include <Adafruit_NeoPixel.h>

#include <map>
#include <string>

Machine smoreBot;

// `modules` populated here. Also checks against rollCall
void configureMachine();

// Module configurations passed here if applicable
void configureModules();

// Station configs populated here
void configureStations();

// Helper for control of inbuilt Neopixel
void setRGB(int r, int g, int b);

channelLabel startButton;
channelLabel eStop;

SerialBoolean startSerial("start", EPHEMERAL);
SerialBoolean eStopSerial("estop", PERSISTENT);
SerialBoolean statusSerial("status", EPHEMERAL);

Adafruit_NeoPixel pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

std::map<std::string, uint8_t> modules;

Dispenser::DispenserHWConfig gc1Config;
Dispenser::DispenserHWConfig chocConfig;
Dispenser::DispenserHWConfig mmConfig;
Oven::OvenHWConfig ovenConfig;
Dispenser::DispenserHWConfig gc2Config;

Belt::BeltHWConfig beltConfig;

#endif