#pragma once
#include <Arduino.h>
#include <SD.h>
#include "Gallery.h"

class FrameStore;

namespace FrameCache {
// Converts source metadata plus display options into a cache identity.
// FAT modification timestamps have 2-second granularity.
struct Identity {
    uint64_t pathHash = 0;
    uint32_t length = 0;
    uint32_t modified = 0;
    uint32_t sampled = 0;
    uint32_t options = 0;
};

Identity identify(const String &path, File &source, const Settings &settings);
enum class LoadResult { Miss, Displayed, DisplayError };
LoadResult loadAndDisplay(const Identity &id);
bool write(const Identity &id, FrameStore &frame);
bool alreadyDisplayed(const Identity &id);
bool markDisplayed(const Identity &id);
void clearDisplayed();
void clearAll(Stream &out);
void printStats(Stream &out);
uint32_t crc32Update(uint32_t current, const uint8_t *data, size_t count);
}
