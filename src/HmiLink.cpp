#include "HmiLink.h"

#include <ArduinoModbus.h>
#include <ArduinoRS485.h>
#include <stdarg.h>
#include <string.h>

#include "Config.h"

namespace hmi {
namespace {

// Full-duplex RS-232 through the transceiver, so there's no DE/RE direction
// pin to toggle between transmit and receive.
RS485Class hmiSerial(Serial1, -1, -1, -1);
ModbusRTUServerClass hmiModbus(hmiSerial);

// True once, clearing the coil so a held press on the screen still yields a
// single edge.
bool readAndClearCoil(int address) {
    if (hmiModbus.coilRead(address) != 1) {
        return false;
    }
    hmiModbus.coilWrite(address, 0);
    return true;
}

}  // namespace

bool begin() {
    if (!hmiModbus.begin(config::kHmiSlaveId, config::kHmiBaud)) {
        return false;
    }
    hmiModbus.configureHoldingRegisters(config::kHmiLineRegister, config::kHmiLineRegisterCount);
    hmiModbus.configureCoils(config::kHmiStartCoil, 2);
    return true;
}

void poll() { hmiModbus.poll(); }

void showLine(const char* fmt, ...) {
    char buffer[config::kHmiLineRegisterCount * 2 + 1];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    // Pad with spaces so a shorter line overwrites a longer previous one
    // instead of leaving its trailing characters on screen.
    for (size_t i = strlen(buffer); i < sizeof(buffer) - 1; i++) {
        buffer[i] = ' ';
    }

    for (int i = 0; i < config::kHmiLineRegisterCount; i++) {
        uint16_t reg = ((uint16_t)(uint8_t)buffer[i * 2] << 8) | (uint8_t)buffer[i * 2 + 1];
        hmiModbus.holdingRegisterWrite(config::kHmiLineRegister + i, reg);
    }
}

bool startEdge() { return readAndClearCoil(config::kHmiStartCoil); }
bool cancelCookEdge() { return readAndClearCoil(config::kHmiCancelCookCoil); }

}  // namespace hmi
