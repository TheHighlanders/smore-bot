#include "stations/Oven.h"

#include "Log.h"

Oven::Oven(std::string name, P1AM& p1, Config config, Timing timing)
    : Station(name, timing),
      m_p1(p1),
      m_config(config),
      m_forceAtTemp(name + "temp", PERSISTENT) {}

void Oven::poll(bool machineRunning) {
    if (!machineRunning) {
        m_p1.writeDiscrete(0, m_config.heater);
        return;
    }

    if (m_config.thermistor.slot == 0) {
        setAtTemp(true);  // No probe fitted: run the oven open-loop.
        return;
    }

    m_temperature = m_p1.readTemperature(m_config.thermistor);

    if (m_temperature < m_config.setpointF - m_config.deadbandF) {
        m_p1.writeDiscrete(1, m_config.heater);
        setAtTemp(false);
    } else {
        // Anywhere at or above the band is hot enough to cook, so an overshoot
        // cannot strand the line waiting for the oven to report ready.
        if (m_temperature > m_config.setpointF + m_config.deadbandF) {
            m_p1.writeDiscrete(0, m_config.heater);
        }
        setAtTemp(true);
    }

    if (m_forceAtTemp.read()) {
        setAtTemp(true);
    }
}

void Oven::onActivate() { m_p1.writeDiscrete(1, m_config.hold); }

void Oven::onRelease() { m_p1.writeDiscrete(0, m_config.hold); }

void Oven::onEStop() {
    m_p1.writeDiscrete(0, m_config.heater);
    m_p1.writeDiscrete(0, m_config.hold);
}

std::string Oven::detail() const {
    if (m_config.thermistor.slot == 0) {
        return "no probe";
    }
    return std::to_string(static_cast<int>(m_temperature)) + "F";
}

void Oven::setAtTemp(bool value) {
    if (value == m_atTemp) {
        return;
    }
    m_atTemp = value;
    logUpdate("%s: %s", name().c_str(), value ? "at temperature" : "heating");
}
