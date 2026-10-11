#pragma once
#include <Arduino.h>
#include <SD.h>

// PPEF v1: 24-byte LE header, 800x480 pixels, two 4-bit palette indices/byte.
namespace NativeFrame {
constexpr uint32_t DATA_SIZE = 800UL * 480UL / 2UL;
constexpr uint32_t HEADER_SIZE = 24;
bool isNative(const String &path);
bool display(File &file);
}
