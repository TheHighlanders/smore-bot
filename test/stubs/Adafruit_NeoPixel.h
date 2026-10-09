#pragma once

// Stands in for the Adafruit_NeoPixel library when testing on a computer.

#include <stdint.h>

#include <vector>

class Adafruit_NeoPixel {
   public:
    Adafruit_NeoPixel(uint16_t count, int16_t /*pin*/) : m_colors(count, 0) {}

    void begin() {}
    void setPixelColor(uint16_t index, uint8_t r, uint8_t g, uint8_t b) {
        m_colors[index] = Color(r, g, b);
    }
    void show() { m_shows++; }
    uint32_t getPixelColor(uint16_t index) const { return m_colors[index]; }
    static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
        return (uint32_t(r) << 16) | (uint32_t(g) << 8) | b;
    }

    int shows() const { return m_shows; }

   private:
    std::vector<uint32_t> m_colors;
    int m_shows = 0;
};
