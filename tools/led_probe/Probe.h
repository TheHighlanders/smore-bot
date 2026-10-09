#ifndef PROBE_H
#define PROBE_H

#include <stdint.h>
#include <string.h>

#include <string>

// The test frame the probe sends, and the reading of the letters the person types.
namespace probe {

// LED k lights its own k-th color, for k = 0 to n-1. Each LED has n bytes.
inline void colorFrame(uint8_t* bytes, size_t size, size_t n, uint8_t level) {
    memset(bytes, 0, size);
    for (size_t k = 0; k < n; k++) {
        bytes[k * n + k] = level;
    }
}

enum class Reading {
    Order,   // Each color once: the strip's color order
    Same,    // All 3 LEDs the same color: the strip may have a fourth color
    NoGood,  // "X": the lights looked wrong
    Unclear
};

// Reads the letters typed for n LEDs: R, G and B, plus W when n is 4.
inline Reading readColors(const std::string& text, size_t n) {
    if (text == "X") {
        return Reading::NoGood;
    }
    std::string colors = n == 4 ? "RGBW" : "RGB";
    if (text.size() == n) {
        bool everyColor = true;
        for (char c : colors) {
            everyColor = everyColor && text.find(c) != std::string::npos;
        }
        if (everyColor) {
            return Reading::Order;
        }
    }
    if (n == 3 && text.size() == 3 && text.find_first_not_of(text[0]) == std::string::npos &&
        std::string("RGBW").find(text[0]) != std::string::npos) {
        return Reading::Same;
    }
    return Reading::Unclear;
}

}  // namespace probe

#endif
