#pragma once

#include <Arduino.h>
#include <SD.h>
#include "Epd.h"

// 4 bits/pixel framebuffer. Uses RAM if available, otherwise SD scratch storage.
class FrameStore {
public:
    bool begin();
    bool pixel(int x, int y, Epd::Color color);
    bool render();
    bool exportPacked(File &target, uint32_t &crc);
    void close();
    bool ok() const { return healthy_; }

private:
    static constexpr uint32_t FRAME_BYTES = uint32_t(Epd::WIDTH) * Epd::HEIGHT / 2;
    static constexpr uint16_t ROW_BYTES = Epd::WIDTH / 2;
    static constexpr uint8_t CACHE_LINES = 4;
    struct CachedRow {
        uint8_t bytes[ROW_BYTES];
        uint16_t index = 0;
        uint32_t lastUse = 0;
        bool valid = false;
        bool dirty = false;
    };

    uint8_t *memory_ = nullptr;
    File scratch_;
    CachedRow cache_[CACHE_LINES];
    uint32_t clock_ = 0;
    bool healthy_ = false;
    bool scratchUsed_ = false;
    bool flushRow(CachedRow &row);
    CachedRow *fetchRow(uint16_t y);
    bool flushAll();
};
