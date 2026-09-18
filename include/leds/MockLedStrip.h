#ifndef MOCKLEDSTRIP_H
#define MOCKLEDSTRIP_H

#include <vector>

#include "leds/LedStrip.h"

// Stands in for the real strip.
class MockLedStrip : public LedStrip {
   public:
    explicit MockLedStrip(uint16_t count) : m_colors(count, Rgb{0, 0, 0}) {}

    void set(uint16_t index, Rgb color) override {
        if (index < m_colors.size()) {
            m_colors[index] = color;
        }
    }
    void show() override { m_shows++; }

    Rgb color(uint16_t index) const { return m_colors[index]; }
    int shows() const { return m_shows; }

   private:
    std::vector<Rgb> m_colors;
    int m_shows = 0;
};

#endif
