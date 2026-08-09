#include "stations/Oven.h"

#include "Arduino.h"

void Oven::update() {
    if (active) {
        // Update tray status tracking
        // TODO: Replace with more robust detection
        trayInside =
            hardware.readDiscrete(config.trayEntrySense) &&
            !(trayInside && hardware.readDiscrete(config.trayExitSense));

        // TODO: Units????
        float temperature = hardware.readTemperature(config.thermistor);

        if (temperature != 0) { // Check for error sentinel
            if (temperature < (config.tempSetpoint - config.tempDeadzone)) {
                // If temperature is too low, activate relay
                hardware.writeDiscrete(1, config.relaySolenoid);
                atTemp = false;
            } else if (temperature >
                       (config.tempSetpoint + config.tempDeadzone)) {
                hardware.writeDiscrete(0, config.relaySolenoid);
            } else {
                atTemp = true;
            }
        }
    }
}

bool Oven::activate(Machine* machine) {
    Serial.printf("Station %s active\r\n", name());
    m_machine = machine;
    active = atTemp && !trayInside;  // Activate if oven is hot, and there is no
                                     // other marshmallow inside.
   
    if(active) {hardware.writeDiscrete(1, config.trayholdSolenoid);}
    return active;
}

void Oven::deactivate() {
    Serial.printf("Station %s inactive\r\n", name());
    hardware.writeDiscrete(0, config.trayholdSolenoid);
    active = false;
}

bool Oven::free() const { return !active; }

void Oven::eStop() {
    hardware.writeDiscrete(0, config.relaySolenoid);

    active = false;
    Serial.printf("STATION %s EMERGENCY STOPPED\r\n", name());
}

std::string Oven::state() const { return (active ? "active" : "inactive"); }