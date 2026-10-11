#include "NativeFrame.h"
#include "Epd.h"
#include "FrameCache.h"
#include <string.h>

namespace {
uint16_t read16(const uint8_t *p) { return uint16_t(p[0]) | uint16_t(p[1]) << 8; }
uint32_t read32(const uint8_t *p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }
}

bool NativeFrame::isNative(const String &path) {
    String name(path); name.toLowerCase();
    return name.endsWith(".epd") || name.endsWith(".edp");
}

bool NativeFrame::display(File &file) {
    uint8_t header[HEADER_SIZE];
    if (!file.seek(0) || file.size() != HEADER_SIZE + DATA_SIZE ||
        file.read(header, HEADER_SIZE) != int(HEADER_SIZE)) {
        Serial.println(F("[EPD] Invalid file size/header")); return false;
    }
    if (memcmp(header, "PPEF", 4) || read16(header + 4) != 1 ||
        read16(header + 6) != Epd::WIDTH || read16(header + 8) != Epd::HEIGHT ||
        header[10] != 1 || header[11] != 0 ||
        read32(header + 12) != DATA_SIZE || read32(header + 20) != 0) {
        Serial.println(F("[EPD] Unsupported version, geometry or pixel layout")); return false;
    }
    const uint32_t expectedCRC = read32(header + 16);
    uint8_t row[Epd::WIDTH / 2];
    uint32_t crc = 0xFFFFFFFFUL;
    // Check CRC and the packed 0..6 palette values BEFORE waking the EPD.
    for (uint16_t y = 0; y < Epd::HEIGHT; ++y) {
        if (file.read(row, sizeof(row)) != int(sizeof(row))) return false;
        for (uint8_t byte : row) if ((byte >> 4) > 6 || (byte & 15) > 6) {
            Serial.println(F("[EPD] Invalid palette index (must be 0..6)")); return false;
        }
        crc = FrameCache::crc32Update(crc, row, sizeof(row));
    }
    if ((crc ^ 0xFFFFFFFFUL) != expectedCRC) {
        Serial.println(F("[EPD] CRC32 mismatch, not refreshing display")); return false;
    }
    if (!file.seek(HEADER_SIZE) || !Epd::begin()) {
        Epd::sleep(); return false;
    }
    bool valid = true;
    Epd::startFrame();
    for (uint16_t y = 0; y < Epd::HEIGHT; ++y) {
        if (file.read(row, sizeof(row)) != int(sizeof(row))) { valid = false; break; }
        Epd::writeRow(row, sizeof(row));
    }
    Epd::endFrame();
    if (valid) valid = Epd::refresh();
    Epd::sleep();
    Serial.println(valid ? F("[EPD] Native frame displayed, zero conversion") : F("[EPD] Display failure"));
    return valid;
}
