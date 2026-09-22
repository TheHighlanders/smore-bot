#ifndef STATION_H
#define STATION_H

#include <stdint.h>

#include <string>

// A timed station. Each phase advances on elapsed time:
//
//   Idle --activate--> Arriving --transitMs--> Working --workMs--> Done
//   Done --deactivate--> Clearing --clearMs--> Idle
//
// reset() returns any phase to Idle with every output de-energized.
// Subclasses supply hardware actions; timing and phase state live here.
class Station {
   public:
    // Work that lasts until the station is reset (e.g. the belt).
    static const uint32_t kContinuous = 0xFFFFFFFFu;

    struct Timing {
        uint32_t transitMs;  // upstream release -> tray arrives here
        uint32_t workMs;     // dispense/cook duration once the tray is here
        uint32_t clearMs;    // release -> tray fully past this station
    };

    Station(std::string name, Timing timing) : m_name(name), m_timing(timing) {}
    virtual ~Station() {}

    void update(uint32_t clock);

    bool activate(uint32_t clock);    // Accepted when free()
    void deactivate(uint32_t clock);  // Release the tray and start clearing
    void reset();                     // Back to Idle with outputs off

    // Bring-up: run this station's actuators through their motions.
    virtual void selfTest() = 0;

    bool free() const { return m_phase == Phase::Idle && ready(); }
    // True once the time remaining until Idle is at most slackMs.
    bool freeWithin(uint32_t clock, uint32_t slackMs) const;
    bool done() const { return m_phase == Phase::Done; }

    const std::string& name() const { return m_name; }
    std::string state() const;

   protected:
    static const uint32_t kPulseMs = 750;  // Actuator hold during selfTest

    virtual void onActivate() {}  // Tray released upstream: extend the stop
    virtual void onArrive() {}    // Tray is here: start working

    // Every tick while Working, with time elapsed in this phase. Return true to
    // finish now (e.g. MM's exit sensor); false lets workMs time out.
    virtual bool onWork(uint32_t /*elapsedMs*/) { return false; }

    virtual void onComplete() {}  // Work finished: park actuators
    virtual void onRelease() {}   // Let the tray go
    virtual void onReset() = 0;   // De-energize every output

    virtual void poll() {}                             // Every tick, in every phase
    virtual bool ready() const { return true; }        // Extra gate on free()
    virtual std::string detail() const { return ""; }  // Appended to state()

   private:
    enum class Phase { Idle, Arriving, Working, Done, Clearing };

    void enter(Phase phase, uint32_t clock);
    uint32_t elapsed(uint32_t clock) const { return clock - m_since; }
    // Time until Idle, or kContinuous if there's no bound (Done, or
    // continuous work).
    uint32_t remainingMs(uint32_t clock) const;

    std::string m_name;
    Timing m_timing;
    Phase m_phase = Phase::Idle;
    uint32_t m_since = 0;
    bool m_stallReported = false;
};

#endif
