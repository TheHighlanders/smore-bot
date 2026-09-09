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

void estop_safes_every_station_and_latches() {
    Line line;
    line.machine.run(true);
    line.machine.startCycle();
    line.run(500);

    line.machine.eStop();
    TEST_ASSERT_TRUE(line.machine.isEStopped());
    TEST_ASSERT_FALSE(line.machine.isRunning());
    TEST_ASSERT_TRUE(line.belt.safed);
    for (FakeStation* station : line.stations()) {
        TEST_ASSERT_TRUE(station->safed);
    }

    line.machine.run(true);
    TEST_ASSERT_FALSE_MESSAGE(line.machine.isRunning(), "e-stop must not be releasable");
    TEST_ASSERT_FALSE(line.machine.startCycle());
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

void skip_advances_the_furthest_along_station() {
    Line line;
    line.machine.run(true);
    line.machine.startCycle();
    line.run(100);

    TEST_ASSERT_TRUE(line.machine.skipStation());
    line.run(10);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, line.gc1.completions, "skip did not finish the work");

    line.run(200000);
    TEST_ASSERT_EQUAL_INT(1, line.gc2.completions);
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
    RUN_TEST(estop_safes_every_station_and_latches);
    RUN_TEST(belt_restarts_across_repeated_holds);
    RUN_TEST(continuous_station_never_completes);
    RUN_TEST(skip_advances_the_furthest_along_station);
    RUN_TEST(belt_clock_survives_millis_rollover);
    return UNITY_END();
}
