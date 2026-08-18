#ifndef MAIN_h
#define MAIN_h

#include "machine/Machine.h"
#include "stations/Dispenser.h"
#include "stations/Oven.h"
#include "SerialBoolean.h"

#include <P1AM.h>

#include <map>
#include <string>

Machine smoreBot;

// `modules` populated here. Also checks against rollCall
void configureMachine();

// Module configurations passed here if applicable
void configureModules();

// Station configs populated here
void configureStations();

channelLabel startButton;
channelLabel eStop;

SerialBoolean startSerial("start", EPHEMERAL);
SerialBoolean eStopSerial("estop", PERSISTENT);

std::map<std::string, uint8_t> modules;

Dispenser::DispenserHWConfig gc1Config;
Dispenser::DispenserHWConfig chocConfig;
Dispenser::DispenserHWConfig mmConfig;
Oven::OvenHWConfig ovenConfig;
Dispenser::DispenserHWConfig gc2Config;

#endif