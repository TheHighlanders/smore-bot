#include "stations/ExampleStation.h"
#include "Arduino.h"

void ExampleStation::update(){
    if(active){
        if(millis() >= (activationTime + 1000)){
            m_machine->onWorkCompleteCallback(this);
            completionTime = millis();
        }
    }
}

bool ExampleStation::activate(Machine* machine){
    Serial.printf("Station %s active\r\n", name());
    m_machine = machine;
    active = true;
    activationTime = millis();
    return true;
}

void ExampleStation::deactivate(){
    Serial.printf("Station %s inactive, time to complete: %d, time idle post-complete: %d\r\n", name(), completionTime - activationTime, completionTime - millis());
    active = false;
}

bool ExampleStation::free() const{
    return !active;
}

void ExampleStation::eStop(){
    active = false;
    Serial.printf("STATION %s EMERGENCY STOPPED\r\n", name());
}

std::string ExampleStation::state() const{
    return (active ? "active" : "inactive");
}