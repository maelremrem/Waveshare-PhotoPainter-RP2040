#pragma once
#include <Arduino.h>

namespace Epd {
constexpr uint16_t WIDTH = 800;
constexpr uint16_t HEIGHT = 480;
enum Color : uint8_t { BLACK=0, WHITE=1, GREEN=2, BLUE=3, RED=4, YELLOW=5, ORANGE=6 };

bool begin();
void startFrame();
void writeRow(const uint8_t *packed4bpp, size_t count);
void endFrame();
bool refresh();
void sleep();
bool clear(Color color = WHITE);
bool testPattern();
bool showLowBattery(uint16_t orientation = 0);
Color nearest(uint8_t red, uint8_t green, uint8_t blue);
}
