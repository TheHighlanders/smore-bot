#include "stations/Belt.h"
#include "Arduino.h"

void Belt::update(){}

bool Belt::activate(Machine* machine){
    Serial.printf("Station %s active\r\n", name());
    m_machine = machine;
    active = true;

    hardware.writeDiscrete(1, config.beltRelay);

    return true;
}

void Belt::deactivate(){
    Serial.printf("Station %s inactive\r\n", name());
    P1.writeDiscrete(0,config.beltRelay);

    active = false;
}

bool Belt::free() const{
    return !active;
}

void Belt::eStop(){
    hardware.writeDiscrete(0, config.beltRelay);

    active = false;
    Serial.printf("STATION %s EMERGENCY STOPPED\r\n", name());
}

std::string Belt::state() const{
    return (active ? "active" : "inactive");
}