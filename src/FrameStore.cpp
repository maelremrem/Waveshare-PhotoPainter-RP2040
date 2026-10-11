#include "FrameStore.h"
#include <stdlib.h>
#include <string.h>

namespace {
constexpr const char *SCRATCH_FILE = "/render.tmp";
}

bool FrameStore::begin() {
    close();
    clock_ = 0;
    for (auto &row : cache_) row = CachedRow{};
    memory_ = static_cast<uint8_t *>(malloc(FRAME_BYTES));
    if (memory_) {
        memset(memory_, 0x11, FRAME_BYTES);
        healthy_ = true;
        Serial.println(F("[FRAME] RAM framebuffer"));
        return true;
    }
    SD.remove(SCRATCH_FILE);
    scratchUsed_ = true;
    scratch_ = SD.open(SCRATCH_FILE, "w+");
    if (!scratch_) return false;
    uint8_t white[ROW_BYTES];
    memset(white, 0x11, sizeof(white));
    for (uint16_t y = 0; y < Epd::HEIGHT; ++y) {
        if (scratch_.write(white, sizeof(white)) != sizeof(white)) {
            close();
            return false;
        }
    }
    scratch_.flush();
    healthy_ = true;
    Serial.println(F("[FRAME] SD framebuffer fallback"));
    return true;
}

bool FrameStore::flushRow(CachedRow &row) {
    if (!row.valid || !row.dirty) return true;
    if (!scratch_.seek(uint32_t(row.index) * ROW_BYTES) ||
        scratch_.write(row.bytes, ROW_BYTES) != ROW_BYTES) {
        healthy_ = false;
        return false;
    }
    row.dirty = false;
    return true;
}

FrameStore::CachedRow *FrameStore::fetchRow(uint16_t y) {
    for (auto &row : cache_) {
        if (row.valid && row.index == y) {
            row.lastUse = ++clock_;
            return &row;
        }
    }
    CachedRow *victim = &cache_[0];
    for (auto &row : cache_) {
        if (!row.valid) { victim = &row; break; }
        if (row.lastUse < victim->lastUse) victim = &row;
    }
    if (!flushRow(*victim)) return nullptr;
    if (!scratch_.seek(uint32_t(y) * ROW_BYTES) ||
        scratch_.read(victim->bytes, ROW_BYTES) != ROW_BYTES) {
        healthy_ = false;
        return nullptr;
    }
    victim->index = y;
    victim->valid = true;
    victim->dirty = false;
    victim->lastUse = ++clock_;
    return victim;
}

bool FrameStore::pixel(int x, int y, Epd::Color color) {
    if (!healthy_) return false;
    if (x < 0 || y < 0 || x >= Epd::WIDTH || y >= Epd::HEIGHT) return true;
    uint8_t *byte;
    if (memory_) {
        byte = &memory_[uint32_t(y) * ROW_BYTES + uint16_t(x / 2)];
    } else {
        CachedRow *row = fetchRow(y);
        if (!row) return false;
        row->dirty = true;
        byte = &row->bytes[x / 2];
    }
    const uint8_t c = static_cast<uint8_t>(color);
    *byte = (x & 1) ? ((*byte & 0xF0U) | c) : ((*byte & 0x0FU) | (c << 4));
    return true;
}

bool FrameStore::flushAll() {
    if (!healthy_) return false;
    if (!memory_) {
        for (auto &row : cache_) if (!flushRow(row)) return false;
        scratch_.flush();
    }
    return true;
}

bool FrameStore::render() {
    if (!flushAll()) return false;
    if (!Epd::begin()) { Epd::sleep(); return false; }
    uint8_t line[ROW_BYTES];
    bool success = true;
    Epd::startFrame();
    for (uint16_t y = 0; y < Epd::HEIGHT; ++y) {
        const uint8_t *bytes = memory_ ? memory_ + uint32_t(y) * ROW_BYTES : line;
        if (!memory_) {
            if (!scratch_.seek(uint32_t(y) * ROW_BYTES) ||
                scratch_.read(line, sizeof(line)) != sizeof(line)) {
                success = false;
                break;
            }
        }
        Epd::writeRow(bytes, ROW_BYTES);
    }
    Epd::endFrame();
    if (success) success = Epd::refresh();
    Epd::sleep();
    return success;
}

void FrameStore::close() {
    if (scratch_) scratch_.close();
    if (scratchUsed_) SD.remove(SCRATCH_FILE);
    scratchUsed_ = false;
    if (memory_) free(memory_);
    memory_ = nullptr;
    healthy_ = false;
}

bool FrameStore::exportPacked(File &target, uint32_t &crc) {
    if (!flushAll()) return false;
    uint8_t line[ROW_BYTES];
    crc = 0xFFFFFFFFUL;
    for (uint16_t y = 0; y < Epd::HEIGHT; ++y) {
        const uint8_t *src = memory_ ? memory_ + uint32_t(y) * ROW_BYTES : line;
        if (!memory_) {
            if (!scratch_.seek(uint32_t(y) * ROW_BYTES) ||
                scratch_.read(line, ROW_BYTES) != ROW_BYTES) return false;
        }
        if (target.write(src, ROW_BYTES) != ROW_BYTES) return false;
        // IEEE CRC32, standard polynomial 0xedb88320.
        for (size_t i = 0; i < ROW_BYTES; ++i) {
            crc ^= src[i];
            for (int bit = 0; bit < 8; ++bit)
                crc = (crc >> 1) ^ ((crc & 1U) ? 0xEDB88320UL : 0);
        }
    }
    crc ^= 0xFFFFFFFFUL;
    return true;
}
