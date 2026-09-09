#include "stations/Dispenser.h"

#include <Arduino.h>

#include "Channel.h"

Dispenser::Dispenser(std::string name, P1AM& p1, Config config, Timing timing)
    : Station(name, timing), m_p1(p1), m_config(config) {
    pinMode(m_config.servoPin, OUTPUT);
    analogWrite(m_config.servoPin, m_config.servoParked);
}

void Dispenser::onActivate() { writeChannel(m_p1, 1, m_config.capture); }

void Dispenser::onArrive() { analogWrite(m_config.servoPin, m_config.servoDispense); }

void Dispenser::onComplete() { analogWrite(m_config.servoPin, m_config.servoParked); }

void Dispenser::onRelease() { writeChannel(m_p1, 0, m_config.capture); }

void Dispenser::onEStop() {
    writeChannel(m_p1, 0, m_config.capture);
    analogWrite(m_config.servoPin, m_config.servoParked);
}
