#include <unity.h>

#include <string>
#include <vector>

#include "Arduino.h"
#include "Probe.h"

uint32_t g_millis = 0;
HostSerial Serial;

namespace {

const size_t kFrameBytes = 4000;
const uint8_t kLevel = 64;

// What a strip shows for these bytes: one letter per LED, or '.' for a dark LED.
// `order` gives the color of each byte in an LED (R, G, B, W).
std::string shows(const std::vector<uint8_t>& bytes, const std::string& order, size_t leds) {
    std::string text;
    for (size_t led = 0; led < leds; led++) {
        char letter = '.';
        for (size_t k = 0; k < order.size(); k++) {
            if (bytes[led * order.size() + k]) letter = order[k];
        }
        text += letter;
    }
    return text;
}

void setUp() { g_millis = 0; }
void tearDown() {}

void the_color_test_shows_every_strips_color_order() {
    const char* orders[] = {"RGB", "RBG", "GRB", "GBR", "BRG", "BGR"};
    for (const char* order : orders) {
        std::vector<uint8_t> bytes(kFrameBytes);
        probe::colorFrame(bytes.data(), bytes.size(), 3, kLevel);

        std::string typed = shows(bytes, order, 3);  // What the person sees, LED 1 first
        TEST_ASSERT_EQUAL_STRING(order, typed.c_str());
        TEST_ASSERT_TRUE(probe::readColors(typed, 3) == probe::Reading::Order);
    }
}

void a_four_color_strip_shows_three_matching_colors_then_its_order() {
    const char* orders[] = {"GRBW", "RGBW", "WRGB"};
    for (const char* order : orders) {
        std::vector<uint8_t> bytes(kFrameBytes);

        probe::colorFrame(bytes.data(), bytes.size(), 3, kLevel);
        std::string three = shows(bytes, order, 3);
        TEST_ASSERT_TRUE_MESSAGE(probe::readColors(three, 3) == probe::Reading::Same, order);

        probe::colorFrame(bytes.data(), bytes.size(), 4, kLevel);
        std::string four = shows(bytes, order, 4);
        TEST_ASSERT_EQUAL_STRING(order, four.c_str());
        TEST_ASSERT_TRUE(probe::readColors(four, 4) == probe::Reading::Order);
    }
}

void typed_colors_are_read_correctly() {
    TEST_ASSERT_TRUE(probe::readColors("GRB", 3) == probe::Reading::Order);
    TEST_ASSERT_TRUE(probe::readColors("X", 3) == probe::Reading::NoGood);
    TEST_ASSERT_TRUE(probe::readColors("GRR", 3) == probe::Reading::Unclear);
    TEST_ASSERT_TRUE(probe::readColors("GRW", 3) == probe::Reading::Unclear);

    TEST_ASSERT_TRUE(probe::readColors("GRBW", 4) == probe::Reading::Order);
    TEST_ASSERT_TRUE(probe::readColors("GGGG", 4) == probe::Reading::Unclear);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(the_color_test_shows_every_strips_color_order);
    RUN_TEST(a_four_color_strip_shows_three_matching_colors_then_its_order);
    RUN_TEST(typed_colors_are_read_correctly);
    return UNITY_END();
}
