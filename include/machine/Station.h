#ifndef STATION_H
#define STATION_H

#include <stdint.h>

#include <string>

// A station with no sensors: every phase advances on elapsed belt time.
//
//   Idle --activate--> Arriving --transitMs--> Working --workMs--> Done
//   Done --deactivate--> Clearing --clearMs--> Idle
//
// Subclasses supply hardware actions only; all timing and state live here, so
// there is exactly one place where a station can report completion.
class Station {
   public:
    // Work that only ends when the station is deactivated (e.g. the belt).
    static const uint32_t kContinuous = 0xFFFFFFFFu;

    struct Timing {
        uint32_t transitMs;  // upstream release -> tray arrives here
        uint32_t workMs;     // dispense/cook duration once the tray is here
        uint32_t clearMs;    // release -> tray fully past this station
    };

    Station(std::string name, Timing timing) : m_name(name), m_timing(timing) {}
    virtual ~Station() {}

    void update(uint32_t clock, bool machineRunning);

    bool activate(uint32_t clock);    // Rejected unless free()
    void deactivate(uint32_t clock);  // Release the tray and start clearing

    // Bring-up: pulse this station's actuators and report its inputs.
    virtual void selfTest() = 0;

    bool free() const { return m_phase == Phase::Idle && ready(); }
    bool done() const { return m_phase == Phase::Done; }

    const std::string& name() const { return m_name; }
    std::string state() const;

   protected:
    static const uint32_t kPulseMs = 750;  // Actuator hold during selfTest

    virtual void onActivate() {}  // Tray released upstream: extend the stop
    virtual void onArrive() {}    // Tray is here: start working
    virtual void onComplete() {}  // Work finished: park actuators

    // Every tick while Working, with time elapsed in this phase. Stations whose
    // work is a sequence of actuator moves drive it from here. Return true to
    // finish immediately (e.g. a sensor confirms the product has left); a
    // station that only needs a timer can ignore elapsedMs and let workMs
    // expire instead.
    virtual bool onWork(uint32_t /*elapsedMs*/) { return false; }
    virtual void onRelease() {}  // Let the tray go

    // Runs every tick in every phase, including Idle. This is what keeps the
    // oven heating while nothing is in it.
    virtual void poll(bool /*machineRunning*/) {}

    virtual bool ready() const { return true; }        // Extra gate on free()
    virtual std::string detail() const { return ""; }  // Appended to state()

   private:
    enum class Phase { Idle, Arriving, Working, Done, Clearing };

    void enter(Phase phase, uint32_t clock);
    uint32_t elapsed(uint32_t clock) const { return clock - m_since; }

    std::string m_name;
    Timing m_timing;
    Phase m_phase = Phase::Idle;
    uint32_t m_since = 0;
    bool m_stallReported = false;
};

#endif
