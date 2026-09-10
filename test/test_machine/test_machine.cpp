#include <unity.h>

#include <vector>

#include "Arduino.h"
#include "FakeStation.h"
#include "machine/Machine.h"

uint32_t g_millis = 0;
HostSerial Serial;

namespace {

const Station::Timing kEntry = {0, 1500, 2000};
const Station::Timing kDispenser = {3000, 1500, 2000};
const Station::Timing kOven = {3000, 45000, 2500};

struct Line {
    FakeStation gc1{"GC1", kEntry};
    FakeStation choc{"CHOC", kDispenser};
    FakeStation mm{"MM", kDispenser};
    FakeStation oven{"OVEN", kOven};
    FakeStation gc2{"GC2", kDispenser};
    FakeStation belt{"BELT", Station::Timing{0, Station::kContinuous, 0}};
    Machine machine;

    Line() {
        machine.configure({&gc1, &choc, &mm, &oven, &gc2}, {&belt});
    }

    std::vector<FakeStation*> stations() { return {&gc1, &choc, &mm, &oven, &gc2}; }

    // Advances wall time in 10 ms steps, optionally leaning on the start button.
    // Counts steps rather than computing an end time, so it works across the
    // millis() rollover.
    void run(uint32_t durationMs, int traysToStart = 0) {
        int started = 0;
        for (uint32_t elapsed = 0; elapsed < durationMs; elapsed += 10) {
            g_millis += 10;
            if (started < traysToStart && machine.startCycle()) started++;
            machine.update();
        }
    }
};

void setUp() { g_millis = 0; }
void tearDown() {}

void one_tray_visits_every_station_in_order() {
    Line line;
    line.machine.run(true);
    line.run(200000, 1);

    for (FakeStation* station : line.stations()) {
        TEST_ASSERT_EQUAL_INT(1, station->activations);
        TEST_ASSERT_EQUAL_INT(1, station->completions);
        TEST_ASSERT_FALSE(station->occupied);
    }
}

void five_trays_never_collide() {
    Line line;
    line.machine.run(true);
    line.run(600000, 5);  // Start button held down the whole run

    for (FakeStation* station : line.stations()) {
        TEST_ASSERT_FALSE_MESSAGE(station->collided, "two trays in one station");
        TEST_ASSERT_EQUAL_INT(5, station->activations);
        TEST_ASSERT_EQUAL_INT(5, station->completions);
        TEST_ASSERT_FALSE(station->occupied);
    }
}

void start_is_ignored_while_entry_station_is_busy() {
    Line line;
    line.machine.run(true);

    TEST_ASSERT_TRUE(line.machine.startCycle());
    TEST_ASSERT_FALSE_MESSAGE(line.machine.startCycle(), "second press must not queue");
    TEST_ASSERT_EQUAL_INT(1, line.gc1.activations);
}

void holding_the_machine_freezes_the_belt_clock() {
    Line line;
    line.machine.run(true);
    line.machine.startCycle();
    line.run(500);  // Part way through GC1's 1500 ms of work

    line.machine.run(false);
    int completionsAtHold = line.gc1.completions;
    line.run(60000);  // A minute of wall time with the belt stopped
    TEST_ASSERT_EQUAL_INT_MESSAGE(completionsAtHold, line.gc1.completions,
                                  "work advanced while the belt was stopped");

    line.machine.run(true);
    line.run(2000);
    TEST_ASSERT_EQUAL_INT(1, line.gc1.completions);
}

void clear_time_gates_the_next_tray() {
    Line line;
    line.machine.run(true);
    line.machine.startCycle();

    // GC1 finishes work at 1500 ms and releases, then clears for 2000 ms.
    line.run(1600);
    TEST_ASSERT_FALSE_MESSAGE(line.machine.startCycle(), "accepted a tray while clearing");
    line.run(2000);
    TEST_ASSERT_TRUE_MESSAGE(line.machine.startCycle(), "still blocked after clear time");
}

void belt_restarts_across_repeated_holds() {
    Line line;
    for (int i = 0; i < 5; i++) {
        line.machine.run(true);
        line.run(50);
        TEST_ASSERT_TRUE_MESSAGE(line.belt.occupied, "belt failed to restart");
        line.machine.run(false);
        line.run(50);
        TEST_ASSERT_FALSE(line.belt.occupied);
    }
}

void continuous_station_never_completes() {
    Line line;
    line.machine.run(true);
    line.run(300000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, line.belt.completions, "belt completed its work");
    TEST_ASSERT_TRUE(line.belt.occupied);
}

// The rewrite exists to prevent a station reporting completion more than once;
// the old design pushed a blocked station onto a held list every tick and
// deadlocked. Block the oven cold and hold MM at Done for a long time.
void completion_is_reported_once_even_when_blocked() {
    Line line;
    line.oven.readyGate = false;  // Oven never comes up to temperature
    line.machine.run(true);
    line.run(120000, 1);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, line.mm.completions,
                                  "blocked station reported completion more than once");
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, line.oven.activations, "tray entered a station not ready");
    TEST_ASSERT_TRUE_MESSAGE(line.mm.occupied, "blocked tray should still be held at MM");
}

void a_station_that_is_not_ready_blocks_the_line() {
    Line line;
    line.oven.readyGate = false;
    line.machine.run(true);
    line.run(120000, 1);
    TEST_ASSERT_EQUAL_INT(0, line.oven.activations);

    line.oven.readyGate = true;  // Comes up to temperature
    line.run(200000);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, line.oven.activations, "line did not resume");
    TEST_ASSERT_EQUAL_INT(1, line.gc2.completions);
}

void hooks_fire_in_order() {
    Line line;
    line.machine.run(true);
    line.run(200000, 1);

    const char* expected[] = {"activate", "arrive", "complete", "release"};
    TEST_ASSERT_EQUAL_INT(4, (int)line.gc1.events.size());
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_STRING(expected[i], line.gc1.events[i].c_str());
    }
}

void work_hook_runs_through_the_work_phase() {
    Line line;
    line.machine.run(true);
    line.machine.startCycle();
    line.run(200000);

    // GC1 works for 1500 ms at 10 ms per tick, so onWork runs many times and
    // never sees an elapsed time past the work duration.
    TEST_ASSERT_TRUE_MESSAGE(line.gc1.works > 100, "onWork barely ran");
    TEST_ASSERT_TRUE_MESSAGE(line.gc1.lastWorkElapsed >= 1500, "work phase ended early");
    TEST_ASSERT_TRUE_MESSAGE(line.gc1.lastWorkElapsed < 1600, "onWork ran past completion");
}

void activate_is_rejected_unless_free() {
    Line line;
    line.machine.run(true);
    TEST_ASSERT_TRUE(line.machine.startCycle());

    // Busy, then clearing, then blocked by a gate: refused in every case.
    TEST_ASSERT_FALSE(line.machine.startCycle());
    line.run(1600);
    TEST_ASSERT_FALSE(line.machine.startCycle());
    line.run(2000);

    line.gc1.readyGate = false;
    TEST_ASSERT_FALSE_MESSAGE(line.machine.startCycle(), "activated a station that is not ready");
    line.gc1.readyGate = true;
    TEST_ASSERT_TRUE(line.machine.startCycle());
}

// MM completes on its exit sensor, not a fixed timer: set the stand-in sensor
// mid-work and confirm the station finishes right away instead of running out
// the clock.
void work_completes_early_when_the_sensor_reports_done() {
    Line line;
    line.machine.run(true);
    line.machine.startCycle();
    line.run(200);  // Partway into GC1's 1500 ms of work

    line.gc1.workDone = true;
    line.run(10);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, line.gc1.completions, "did not complete on the sensor");
    TEST_ASSERT_TRUE_MESSAGE(line.gc1.lastWorkElapsed < 1500, "waited for the timer instead");
}

// The debug/bring-up entry point dispatches a typed station name straight to
// Machine::selfTestNamed(); confirm it runs that one station's sequence and
// none of the others, including the continuous one (the belt).
void self_test_named_runs_only_that_stations_sequence() {
    Line line;

    TEST_ASSERT_TRUE(line.machine.selfTestNamed("CHOC"));
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, line.choc.selfTests, "CHOC did not run its sequence");
    TEST_ASSERT_EQUAL_INT(0, line.gc1.selfTests);
    TEST_ASSERT_EQUAL_INT(0, line.mm.selfTests);
    TEST_ASSERT_EQUAL_INT(0, line.oven.selfTests);
    TEST_ASSERT_EQUAL_INT(0, line.gc2.selfTests);
    TEST_ASSERT_EQUAL_INT(0, line.belt.selfTests);

    TEST_ASSERT_TRUE(line.machine.selfTestNamed("BELT"));
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, line.belt.selfTests, "continuous station not reachable by name");

    TEST_ASSERT_FALSE_MESSAGE(line.machine.selfTestNamed("NOPE"), "found a station that doesn't exist");
}

// Bring-up types whatever case a station's name happens to be in.
void self_test_named_ignores_case() {
    Line line;

    TEST_ASSERT_TRUE_MESSAGE(line.machine.selfTestNamed("gc1"), "lowercase did not match");
    TEST_ASSERT_TRUE_MESSAGE(line.machine.selfTestNamed("Choc"), "mixed case did not match");
    TEST_ASSERT_TRUE_MESSAGE(line.machine.selfTestNamed("BELT"), "uppercase did not match");

    TEST_ASSERT_EQUAL_INT(1, line.gc1.selfTests);
    TEST_ASSERT_EQUAL_INT(1, line.choc.selfTests);
    TEST_ASSERT_EQUAL_INT(1, line.belt.selfTests);
}

void belt_clock_survives_millis_rollover() {
    Line line;
    g_millis = 0xFFFFFF00u;  // ~256 ms before rollover
    line.machine.run(true);
    line.machine.startCycle();
    line.run(200000, 0);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, line.gc2.completions, "line stalled across rollover");
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(one_tray_visits_every_station_in_order);
    RUN_TEST(five_trays_never_collide);
    RUN_TEST(start_is_ignored_while_entry_station_is_busy);
    RUN_TEST(holding_the_machine_freezes_the_belt_clock);
    RUN_TEST(clear_time_gates_the_next_tray);
    RUN_TEST(belt_restarts_across_repeated_holds);
    RUN_TEST(continuous_station_never_completes);
    RUN_TEST(completion_is_reported_once_even_when_blocked);
    RUN_TEST(a_station_that_is_not_ready_blocks_the_line);
    RUN_TEST(hooks_fire_in_order);
    RUN_TEST(work_hook_runs_through_the_work_phase);
    RUN_TEST(activate_is_rejected_unless_free);
    RUN_TEST(work_completes_early_when_the_sensor_reports_done);
    RUN_TEST(self_test_named_runs_only_that_stations_sequence);
    RUN_TEST(self_test_named_ignores_case);
    RUN_TEST(belt_clock_survives_millis_rollover);
    return UNITY_END();
}
