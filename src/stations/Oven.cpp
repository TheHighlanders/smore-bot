#include "stations/Oven.h"

#include "Arduino.h"

void Oven::update() {
    if (active) {
        // Update tray status tracking
        // TODO: Replace with more robust detection
        trayInside =
            hardware.readDiscrete(config.trayEntrySense) &&
            !(trayInside && hardware.readDiscrete(config.trayExitSense));


        if(hardware.readDiscrete(config.trayExitSense) || exitSerial.read()){
            m_machine->onWorkCompleteCallback(this);
        }
    }

    if (m_machine->isRunning()) {
        // TODO: Units????
        temperature = hardware.readTemperature(config.thermistor);

        // if (temperature != 0) {  // Check for error sentinel
            if (temperature < (config.tempSetpoint - config.tempDeadzone)) {
                // If temperature is too low, activate relay
                hardware.writeDiscrete(1, config.relaySolenoid);
                if(atTemp){
                    // Falling Edge Detection
                    logUpdate("Update: %s: Heating", name().c_str());
                }
                atTemp = false;
            } else if (temperature >
                       (config.tempSetpoint + config.tempDeadzone)) {
                hardware.writeDiscrete(0, config.relaySolenoid);
            } else {
                if(!atTemp){
                    // Check Edge Detection
                    logUpdate("Update: %s: At Temp", name().c_str());
                }
                atTemp = true;
            }
        // }
        // logInfo("Oven Temp: %f", temperature);
        if(tempSerial.read()){
            atTemp = true;
        }
    } else {
        hardware.writeDiscrete(0, config.relaySolenoid); // Disable Heating when not running
    }
}

bool Oven::activate(Machine* machine) {
    m_machine = machine;
    active = atTemp && !trayInside;  // Activate if oven is hot, and there
                                     // is no other marshmallow inside.

    if (active) {
        hardware.writeDiscrete(1, config.trayholdSolenoid);
        logInfo("Station %s active", name().c_str());
        Machine::timer.in(round(cookTime*1000), ovenTimerCallback, this); // Invoke ovenTimerCallback with `this` as arguement in cookTime seconds.
    } else {
        logError("Station %s unable to activate", name().c_str());
    }
    return active;
}

void Oven::deactivate() {
    logInfo("Station %s inactive", name().c_str());
    hardware.writeDiscrete(0, config.trayholdSolenoid);
    active = false;
}

bool Oven::free() const { 
    logInfo("Oven Not Free: %s %s %s", (active ? "Active" : "Inactive"), (atTemp ? "At Temp" : "Not At Temp"), (trayInside ? "Tray Inside" : "No Tray Inside"));
    return !active && atTemp && !trayInside; 
} // An Oven is free if it is at temperature, empty, and not otherwise active (should be redundant)

void Oven::eStop() {
    // Depower Oven
    hardware.writeDiscrete(0, config.relaySolenoid);

    // Release Tray Solenoid
    hardware.writeDiscrete(0, config.trayholdSolenoid);

    active = false;
    logError("STATION %s EMERGENCY STOPPED", name().c_str());
}

bool Oven::ovenTimerCallback(void* argument){
    //Used to allow callback function to be static
    Oven* self = static_cast<Oven*>(argument);

    logUpdate("Update: %s: Cook Complete", self->name().c_str());

    self->m_machine->onWorkCompleteCallback(self);
    return false;
}

std::string Oven::state() const { return std::string(active ? "active" : "inactive") + ", Oven Temp: " + std::to_string(round(temperature)); }