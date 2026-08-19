#include "stations/Dispenser.h"
#include "Arduino.h"

void Dispenser::update(){
    if(active){
        // Poll Sensor, and wait for tray to be captured
        if(!dispensing && (hardware.readDiscrete(config.traySense) || detectSerial.read())){
            logUpdate("Update: %s: Captured Tray", name().c_str());
            // Tray captured, dispense
            analogWrite(config.dispensePWM, config.dispensePWMActive);
            dispensing = true;
            Machine::timer.in(1000, dispenseTimerCallback, this);
        }
    }
}

bool Dispenser::activate(Machine* machine){
    logInfo("Station %s active", name().c_str());
    m_machine = machine;
    active = true;

    hardware.writeDiscrete(1, config.captureSolenoid); // Capture Tray

    return true;
}

void Dispenser::deactivate(){
    logInfo("Station %s inactive", name().c_str());
    analogWrite(config.dispensePWM, config.dispensePWMInactive);
    hardware.writeDiscrete(0, config.captureSolenoid); // Release Tray

    active = false;
    dispensing = false;
}

bool Dispenser::dispenseTimerCallback(void* argument){
    //Used to allow callback function to be static
    Dispenser* self = static_cast<Dispenser*>(argument);
    // Serial.printf("Update: %s: Completed Dispense\r\n", self->name().c_str());
    logUpdate("Update: %s: Completed Dispense", self->name().c_str());
    // Write to the PWM to command dispenser in, update state, and fire complete callback
    analogWrite(self->config.dispensePWM, self->config.dispensePWMInactive);
    self->dispensing = false;
    self->m_machine->onWorkCompleteCallback(self);
    return false; // Run timer once
}

bool Dispenser::free() const{
    return !active;
}

void Dispenser::eStop(){
    hardware.writeDiscrete(0, config.captureSolenoid);
    // Leave dispenser servo where it is.

    active = false;
    logError("STATION %s EMERGENCY STOPPED", name().c_str());
}

std::string Dispenser::state() const{
    return (active ? "active" : "inactive");
}