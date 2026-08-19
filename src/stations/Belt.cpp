#include "stations/Belt.h"
#include "Arduino.h"

void Belt::update(){}

bool Belt::activate(Machine* machine){
    logInfo("Station %s active", name().c_str());
    m_machine = machine;
    active = true;

    hardware.writeDiscrete(1, config.beltRelay);

    return true;
}

void Belt::deactivate(){
    logInfo("Station %s inactive", name().c_str());
    P1.writeDiscrete(0,config.beltRelay);

    active = false;
}

bool Belt::free() const{
    return !active;
}

void Belt::eStop(){
    hardware.writeDiscrete(0, config.beltRelay);

    active = false;
    logError("STATION %s EMERGENCY STOPPED", name().c_str());
}

std::string Belt::state() const{
    return (active ? "active" : "inactive");
}