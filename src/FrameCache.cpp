#include "FrameCache.h"
#include "FrameStore.h"
#include "Epd.h"
#include "NativeFrame.h"
#include <stdio.h>
#include <string.h>

namespace {
constexpr uint32_t IMAGE_SIZE = uint32_t(Epd::WIDTH) * Epd::HEIGHT / 2U;
constexpr uint32_t VERSION = 5; // Bump when palette, layout or scaling algorithm changes.
constexpr char CACHE_DIR[] = "/cache";
constexpr char LAST_PATH[] = "/last.epd";
constexpr char LAST_TMP[] = "/last.tmp";
constexpr char WRITE_TMP[] = "/cache/write.tmp";

struct __attribute__((packed)) Header {
    char magic[4];
    uint32_t version;
    uint32_t frameBytes;
    uint64_t pathHash;
    uint32_t fileBytes;
    uint32_t modified;
    uint32_t sampled;
    uint32_t options;
    uint32_t dataCRC;
};
static_assert(sizeof(Header) == 40, "Unexpected cache header padding");

uint32_t hash32(uint64_t v) { return uint32_t(v) ^ uint32_t(v >> 32); }
String pathFor(const FrameCache::Identity &id) {
    char name[28];
    snprintf(name, sizeof(name), "/cache/%08lx.epc", (unsigned long)hash32(id.pathHash));
    return String(name);
}
Header expected(const FrameCache::Identity &id) {
    Header h{};
    memcpy(h.magic, "EP7C", 4);
    h.version = VERSION;
    h.frameBytes = IMAGE_SIZE;
    h.pathHash = id.pathHash;
    h.fileBytes = id.length;
    h.modified = id.modified;
    h.sampled = id.sampled;
    h.options = id.options;
    return h;
}
bool matches(const Header &a, const Header &b) {
    return !memcmp(a.magic, b.magic, 4) && a.version == b.version &&
        a.frameBytes == b.frameBytes && a.pathHash == b.pathHash &&
        a.fileBytes == b.fileBytes && a.modified == b.modified &&
        a.sampled == b.sampled && a.options == b.options;
}
bool headerRead(File &f, Header &h) {
    return f.read(reinterpret_cast<uint8_t *>(&h), sizeof(h)) == int(sizeof(h));
}
bool headerWrite(File &f, const Header &h) {
    return f.write(reinterpret_cast<const uint8_t *>(&h), sizeof(h)) == sizeof(h);
}
}

namespace FrameCache {
uint32_t crc32Update(uint32_t current, const uint8_t *data, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        current ^= data[i];
        for (uint8_t bit = 0; bit < 8; ++bit)
            current = (current >> 1) ^ ((current & 1U) ? 0xEDB88320UL : 0);
    }
    return current;
}
Identity identify(const String &path, File &source, const Settings &settings) {
    Identity id;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < path.length(); ++i) {
        hash ^= uint8_t(path[i]);
        hash *= UINT64_C(1099511628211);
    }
    id.pathHash = hash;
    id.length = source.size();
    id.modified = uint32_t(source.getLastWrite());
    // Sample file ends to detect replacements with equal size/FAT timestamp.
    const uint32_t original = source.position();
    const uint32_t size = uint32_t(source.size());
    const uint32_t chunk = min(size, uint32_t(1024));
    uint8_t bytes[128];
    uint32_t digest = 2166136261UL;
    for (uint8_t part = 0; part < (size > chunk ? 2 : 1); ++part) {
        const uint32_t start = part ? size - chunk : 0;
        if (!source.seek(start)) { source.seek(original); return id; }
        uint32_t remaining = chunk;
        while (remaining) {
            const size_t n = min(uint32_t(sizeof(bytes)), remaining);
            if (source.read(bytes, n) != int(n)) { source.seek(original); return id; }
            for (size_t i = 0; i < n; ++i) {
                digest ^= bytes[i];
                digest *= 16777619UL;
            }
            remaining -= n;
        }
    }
    source.seek(original);
    id.sampled = digest;
    id.options = NativeFrame::isNative(path) ? 0 :
                 uint32_t(settings.orientation) | (uint32_t(settings.jpegCover) << 16);
    return id;
}

LoadResult loadAndDisplay(const Identity &id) {
    const String path = pathFor(id);
    File f = SD.open(path.c_str(), FILE_READ);
    if (!f) return LoadResult::Miss;
    Header h{};
    const Header check = expected(id);
    if (!headerRead(f, h) || !matches(h, check) ||
        f.size() != IMAGE_SIZE + sizeof(Header)) {
        f.close();
        SD.remove(path.c_str());
        Serial.println(F("[CACHE] Stale or incomplete cache; regenerate"));
        return LoadResult::Miss;
    }
    if (!Epd::begin()) {
        f.close(); Epd::sleep();
        return LoadResult::DisplayError;
    }
    // Single pass: verify CRC while sending rows to EPD RAM. Do not refresh on corruption.
    uint8_t row[Epd::WIDTH / 2];
    uint32_t crc = 0xFFFFFFFFUL;
    bool valid = true;
    Epd::startFrame();
    for (uint16_t y = 0; y < Epd::HEIGHT; ++y) {
        if (f.read(row, sizeof(row)) != int(sizeof(row))) {
            valid = false;
            break;
        }
        crc = crc32Update(crc, row, sizeof(row));
        Epd::writeRow(row, sizeof(row));
    }
    Epd::endFrame();
    f.close();
    valid = valid && ((crc ^ 0xFFFFFFFFUL) == h.dataCRC);
    bool success = false;
    if (valid) success = Epd::refresh();
    Epd::sleep();
    if (!valid) {
        SD.remove(path.c_str());
        Serial.println(F("[CACHE] CRC/read error; regenerate"));
        return LoadResult::Miss;
    }
    if (success) Serial.println(F("[CACHE] HIT: 192 KB direct to EPD"));
    return success ? LoadResult::Displayed : LoadResult::DisplayError;
}

bool write(const Identity &id, FrameStore &frame) {
    if (!SD.exists(CACHE_DIR) && !SD.mkdir(CACHE_DIR)) return false;
    SD.remove(WRITE_TMP);
    File f = SD.open(WRITE_TMP, "w+");
    if (!f) return false;
    Header h = expected(id);
    bool ok = headerWrite(f, h);
    uint32_t crc = 0;
    if (ok) ok = frame.exportPacked(f, crc);
    h.dataCRC = crc;
    if (ok) ok = f.seek(0) && headerWrite(f, h);
    if (ok) f.flush();
    f.close();
    if (!ok) { SD.remove(WRITE_TMP); return false; }
    const String target = pathFor(id);
    SD.remove(target.c_str());
    if (!SD.rename(WRITE_TMP, target.c_str())) {
        SD.remove(WRITE_TMP);
        return false;
    }
    Serial.printf("[CACHE] STORED: %s (%lu bytes)\n", target.c_str(),
                  (unsigned long)(sizeof(Header) + IMAGE_SIZE));
    return true;
}

bool alreadyDisplayed(const Identity &id) {
    File f = SD.open(LAST_PATH, FILE_READ);
    if (!f) return false;
    Header h{};
    const bool result = f.size() == sizeof(Header) && headerRead(f, h) &&
        matches(h, expected(id));
    f.close();
    return result;
}

bool markDisplayed(const Identity &id) {
    File f = SD.open(LAST_TMP, "w+");
    if (!f) return false;
    const Header h = expected(id);
    const bool ok = headerWrite(f, h);
    if (ok) f.flush();
    f.close();
    if (!ok) { SD.remove(LAST_TMP); return false; }
    SD.remove(LAST_PATH);
    if (!SD.rename(LAST_TMP, LAST_PATH)) return false;
    return true;
}
void clearDisplayed() {
    SD.remove(LAST_PATH);
    SD.remove(LAST_TMP);
}
void clearAll(Stream &out) {
    if (!SD.exists(CACHE_DIR)) {
        clearDisplayed();
        out.println(F("Cache already empty")); return;
    }
    File dir = SD.open(CACHE_DIR);
    if (!dir || !dir.isDirectory()) { out.println(F("Cache unavailable")); return; }
    dir.close();
    uint32_t removed = 0;
    for (;;) {
        File scan = SD.open(CACHE_DIR);
        if (!scan || !scan.isDirectory()) break;
        String victim;
        while (File file = scan.openNextFile()) {
            if (!file.isDirectory()) {
                String name = file.name();
                String lower = name; lower.toLowerCase();
                if (lower.endsWith(".epc") || lower.endsWith(".tmp")) {
                    victim = name.startsWith("/") ? name : String(CACHE_DIR) + "/" + name;
                    file.close();
                    break;
                }
            }
            file.close();
        }
        scan.close();
        if (!victim.length() || !SD.remove(victim.c_str())) break;
        ++removed;
    }
    clearDisplayed();
    out.printf("Removed %lu cache files\n", (unsigned long)removed);
}
void printStats(Stream &out) {
    File dir = SD.open(CACHE_DIR);
    if (!dir || !dir.isDirectory()) {
        out.println(F("Cache: empty")); return;
    }
    uint32_t count = 0;
    uint64_t bytes = 0;
    while (File file = dir.openNextFile()) {
        String name = file.name(); name.toLowerCase();
        if (!file.isDirectory() && name.endsWith(".epc")) {
            ++count;
            bytes += file.size();
        }
        file.close();
    }
    dir.close();
    out.printf("Cache: %lu image(s), %llu bytes, %s\n",
               (unsigned long)count, (unsigned long long)bytes,
               SD.exists(LAST_PATH) ? "last-image marker present" : "no marker");
}
}
