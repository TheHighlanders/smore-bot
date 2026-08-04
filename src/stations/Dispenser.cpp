#include "stations/Dispenser.h"
#include "Arduino.h"

void Dispenser::update(){
    if(active){
        // Poll Sensor, and wait for tray to be captured
        if(hardware.readDiscrete(config.traySense)){
            // Tray captured, dispense
            hardware.writePWMDuty(config.dispensePWMActive, config.dispensePWM);
            dispensing = true;
        }

        if(dispensing){
            //TODO: Add Timer
            hardware.writePWMDuty(config.dispensePWMInactive, config.dispensePWM);
            dispensing = false;
        }
    }
}

bool Dispenser::activate(Machine* machine){
    Serial.printf("Station %s active\r\n", name());
    m_machine = machine;
    active = true;

    hardware.writeDiscrete(1, config.captureSolenoid); // Capture Tray

    return true;
}

void Dispenser::deactivate(){
    Serial.printf("Station %s inactive\r\n", name());

    hardware.writePWMDuty(config.dispensePWMInactive, config.dispensePWM);
    hardware.writeDiscrete(0, config.captureSolenoid); // Release Tray

    active = false;
    dispensing = false;
}

bool Dispenser::free() const{
    return !active;
}

void Dispenser::eStop(){
    hardware.writeDiscrete(0, config.captureSolenoid);
    // Leave dispenser servo where it is.

    active = false;
    Serial.printf("STATION %s EMERGENCY STOPPED\r\n", name());
}

std::string Dispenser::state() const{
    return (active ? "active" : "inactive");
}