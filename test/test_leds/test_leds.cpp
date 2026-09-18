#include <unity.h>

#include <string>

#include "Arduino.h"
#include "FakeStation.h"
#include "leds/MockLedStrip.h"
#include "leds/StatusLeds.h"

uint32_t g_millis = 0;
HostSerial Serial;

namespace {

const Rgb kOff = {0, 0, 0};
const Rgb kBlue = {0, 0, 255};
const Rgb kGreen = {0, 255, 0};
const Rgb kYellow = {255, 255, 0};
const Rgb kRed = {255, 0, 0};

// Clearing is yellow so it differs from idle and arriving.
const StatusLeds::Config kConfig = {kBlue, kGreen, kYellow, 3, 100};
const StatusLeds::Pulse kPulse = {kGreen, 2000};
const StatusLeds::Pulse kFastPulse = {kRed, 400};

const Station::Timing kTiming = {1000, 1000, 1000};

bool same(Rgb a, Rgb b) { return a.r == b.r && a.g == b.g && a.b == b.b; }

void expectColor(Rgb expected, Rgb actual) {
    TEST_ASSERT_EQUAL_UINT8(expected.r, actual.r);
    TEST_ASSERT_EQUAL_UINT8(expected.g, actual.g);
    TEST_ASSERT_EQUAL_UINT8(expected.b, actual.b);
}

// A 16-LED strip:
//   LEDs 0, 11, 12, 15  belt (the LEDs left over)
//   LEDs 1-10           station A
//   LEDs 13-14          station B, shorter than a band
struct Fixture {
    MockLedStrip strip{16};
    FakeStation a{"A", kTiming};
    FakeStation b{"B", kTiming};
    FakeStation belt{"BELT", Station::Timing{0, Station::kContinuous, 0}};
    StatusLeds leds{strip,
                    kConfig,
                    {{&belt, kPulse, {0, 16}}, {&a, kPulse, {1, 10}}, {&b, kFastPulse, {13, 2}}}};

    std::string pattern(uint16_t first, uint16_t count) const {
        const struct {
            Rgb color;
            char letter;
        } kLetters[] = {{kOff, '.'}, {kBlue, 'B'}, {kGreen, 'G'}, {kYellow, 'Y'}, {kRed, 'R'}};

        std::string text;
        for (uint16_t i = first; i < first + count; i++) {
            char letter = '?';
            for (const auto& entry : kLetters) {
                if (same(strip.color(i), entry.color)) letter = entry.letter;
            }
            text += letter;
        }
        return text;
    }

    std::string stationA() const { return pattern(1, 10); }
    std::string stationB() const { return pattern(13, 2); }
    std::string beltLeds() const { return pattern(0, 1) + pattern(11, 2) + pattern(15, 1); }

    void workA() {
        a.activate(0);
        a.update(1000);  // Tray arrives
    }
};

void setUp() { g_millis = 0; }
void tearDown() {}

void idle_stations_and_the_belt_are_blue() {
    Fixture f;
    f.leds.update(0);

    TEST_ASSERT_EQUAL_STRING("BBBBBBBBBB", f.stationA().c_str());
    TEST_ASSERT_EQUAL_STRING("BB", f.stationB().c_str());
    TEST_ASSERT_EQUAL_STRING("BBBB", f.beltLeds().c_str());
}

void the_belt_lights_the_leds_left_over_by_stations() {
    Fixture f;
    f.belt.activate(0);
    f.belt.update(0);     // Belt runs
    f.leds.update(1000);  // Half a pulse: brightest

    TEST_ASSERT_EQUAL_STRING("GGGG", f.beltLeds().c_str());
    TEST_ASSERT_EQUAL_STRING("BBBBBBBBBB", f.stationA().c_str());
    TEST_ASSERT_EQUAL_STRING("BB", f.stationB().c_str());
}

void a_waiting_station_shows_a_band_moving_from_start_to_end() {
    Fixture f;
    f.a.activate(0);

    f.leds.update(0);
    TEST_ASSERT_EQUAL_STRING("GGG.......", f.stationA().c_str());
    f.leds.update(100);
    TEST_ASSERT_EQUAL_STRING(".GGG......", f.stationA().c_str());
    f.leds.update(700);
    TEST_ASSERT_EQUAL_STRING(".......GGG", f.stationA().c_str());
    f.leds.update(800);  // Back to the start
    TEST_ASSERT_EQUAL_STRING("GGG.......", f.stationA().c_str());

    TEST_ASSERT_EQUAL_STRING("BB", f.stationB().c_str());  // B stays idle
}

void a_band_fills_a_station_shorter_than_the_band() {
    Fixture f;
    f.b.activate(0);
    f.leds.update(100);

    TEST_ASSERT_EQUAL_STRING("GG", f.stationB().c_str());
}

void a_working_station_pulses_from_off_to_full() {
    Fixture f;
    f.workA();

    f.leds.update(0);
    TEST_ASSERT_EQUAL_STRING("..........", f.stationA().c_str());
    f.leds.update(500);  // Rising
    expectColor(Rgb{0, 127, 0}, f.strip.color(1));
    f.leds.update(1000);
    TEST_ASSERT_EQUAL_STRING("GGGGGGGGGG", f.stationA().c_str());
    f.leds.update(1500);  // Falling
    expectColor(Rgb{0, 127, 0}, f.strip.color(1));
    f.leds.update(2000);
    TEST_ASSERT_EQUAL_STRING("..........", f.stationA().c_str());
}

void a_station_can_pulse_in_its_own_color_and_rate() {
    Fixture f;
    f.workA();
    f.b.activate(0);
    f.b.update(1000);

    f.leds.update(200);  // B is at full; A has barely started
    TEST_ASSERT_EQUAL_STRING("RR", f.stationB().c_str());
    expectColor(Rgb{0, 51, 0}, f.strip.color(1));
}

void a_finished_station_stays_solid_in_its_pulse_color() {
    Fixture f;
    f.workA();
    f.a.update(2000);  // Work ends, tray still held

    f.leds.update(0);
    TEST_ASSERT_EQUAL_STRING("GGGGGGGGGG", f.stationA().c_str());
    f.leds.update(1234);
    TEST_ASSERT_EQUAL_STRING("GGGGGGGGGG", f.stationA().c_str());
}

void a_clearing_station_shows_a_band_in_the_clearing_color() {
    Fixture f;
    f.workA();
    f.a.update(2000);
    f.a.deactivate(2000);  // Tray leaves

    f.leds.update(0);
    TEST_ASSERT_EQUAL_STRING("YYY.......", f.stationA().c_str());
}

void every_update_lights_the_strip() {
    Fixture f;
    f.leds.update(0);
    f.leds.update(10);

    TEST_ASSERT_EQUAL_INT(2, f.strip.shows());
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(idle_stations_and_the_belt_are_blue);
    RUN_TEST(the_belt_lights_the_leds_left_over_by_stations);
    RUN_TEST(a_waiting_station_shows_a_band_moving_from_start_to_end);
    RUN_TEST(a_band_fills_a_station_shorter_than_the_band);
    RUN_TEST(a_working_station_pulses_from_off_to_full);
    RUN_TEST(a_station_can_pulse_in_its_own_color_and_rate);
    RUN_TEST(a_finished_station_stays_solid_in_its_pulse_color);
    RUN_TEST(a_clearing_station_shows_a_band_in_the_clearing_color);
    RUN_TEST(every_update_lights_the_strip);
    return UNITY_END();
}
