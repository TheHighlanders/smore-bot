#include "stations/Dispenser.h"
#include "Arduino.h"

void Dispenser::update(){
    if(active){
        // Poll Sensor, and wait for tray to be captured
        if(!dispensing && hardware.readDiscrete(config.traySense)){
            // Tray captured, dispense
            analogWrite(config.dispensePWM, config.dispensePWMActive);
            dispensing = true;
            Machine::timer.in(5000, dispenseTimerCallback, this);
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
    analogWrite(config.dispensePWM, config.dispensePWMInactive);
    hardware.writeDiscrete(0, config.captureSolenoid); // Release Tray

    active = false;
    dispensing = false;
}

bool Dispenser::dispenseTimerCallback(void* argument){
    //Used to allow callback function to be static
    Dispenser* self = static_cast<Dispenser*>(argument);

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
    Serial.printf("STATION %s EMERGENCY STOPPED\r\n", name());
}

std::string Dispenser::state() const{
    return (active ? "active" : "inactive");
}