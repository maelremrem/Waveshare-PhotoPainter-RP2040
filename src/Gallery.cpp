#include "Gallery.h"
#include "Board.h"
#include "Epd.h"
#include "FrameStore.h"
#include "FrameCache.h"
#include "NativeFrame.h"
#include <SPI.h>
#include <SD.h>
#include <JPEGDEC.h>
#include <tjpgd.h>
#include <math.h>
#include <vector>
#include <algorithm>

namespace {
uint32_t images = 0;
std::vector<String> sortedNames;
uint32_t shuffleStep = 1;
uint32_t current = 0;
bool mounted = false;
String activeDirectory = "/pic";


String baseName(const String &path) {
    const int slash = path.lastIndexOf('/');
    return path.substring(slash + 1);
}
uint32_t gcd(uint32_t a, uint32_t b) {
    while (b) { uint32_t t = a % b; a = b; b = t; }
    return a;
}
void loadShuffleStep() {
    shuffleStep = 1;
    if (images <= 2) return;
    File f = SD.open("/shuffle.step", FILE_READ);
    if (f) { shuffleStep = uint32_t(f.parseInt()); f.close(); }
    if (shuffleStep > 1 && shuffleStep < images && gcd(shuffleStep, images) == 1) return;
    shuffleStep = uint32_t(random(2, images));
    while (gcd(shuffleStep, images) != 1) {
        if (++shuffleStep >= images) shuffleStep = 1;
    }
    SD.remove("/shuffle.step");
    File out = SD.open("/shuffle.step", FILE_WRITE);
    if (out) { out.println(shuffleStep); out.close(); }
}

bool isJpeg(const String &name) {
    String lower = name;
    lower.toLowerCase();
    return lower.endsWith(".jpg") || lower.endsWith(".jpeg");
}
bool isImage(const String &name) {
    const int slash = name.lastIndexOf('/');
    const String base = name.substring(slash + 1);
    if (base.length() < 5 || base.startsWith(".")) return false;
    String lower = base;
    lower.toLowerCase();
    return lower.endsWith(".bmp") || isJpeg(lower) || NativeFrame::isNative(lower);
}
uint16_t le16(const uint8_t *b) { return uint16_t(b[0]) | (uint16_t(b[1]) << 8); }
uint32_t le32(const uint8_t *b) { return uint32_t(b[0]) | (uint32_t(b[1]) << 8) | (uint32_t(b[2]) << 16) | (uint32_t(b[3]) << 24); }

struct BmpInfo {
    uint32_t offset = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t rowSize = 0;
    bool topDown = false;
};

bool readBmpHeader(File &f, BmpInfo &info) {
    uint8_t h[54];
    if (f.read(h, sizeof(h)) != int(sizeof(h))) return false;
    if (h[0] != 'B' || h[1] != 'M') return false;
    if (le32(h + 14) < 40 || le16(h + 26) != 1 ||
        le16(h + 28) != 24 || le32(h + 30) != 0) return false;
    const int32_t width = int32_t(le32(h + 18));
    const int32_t height = int32_t(le32(h + 22));
    if (width < 1 || width > 16384 || height == 0 ||
        height > 16384 || height < -16384) return false;
    info.width = uint32_t(width);
    info.height = uint32_t(height < 0 ? -height : height);
    info.offset = le32(h + 10);
    info.rowSize = (info.width * 3U + 3U) & ~3UL;
    info.topDown = height < 0;
    return info.offset >= 54 &&
        uint64_t(info.offset) + uint64_t(info.rowSize) * info.height <= f.size();
}

struct Fit {
    int width, height, originX, originY;
};
Fit calculateFit(uint32_t srcW, uint32_t srcH, uint16_t orientation, bool cover) {
    const bool portrait = orientation == 90 || orientation == 270;
    const int targetW = portrait ? Epd::HEIGHT : Epd::WIDTH;
    const int targetH = portrait ? Epd::WIDTH : Epd::HEIGHT;
    const float scaleW = float(targetW) / srcW;
    const float scaleH = float(targetH) / srcH;
    const float scale = cover ? max(scaleW, scaleH) : min(scaleW, scaleH);
    const int w = max(1, int(lroundf(srcW * scale)));
    const int h = max(1, int(lroundf(srcH * scale)));
    return {w, h, (targetW - w) / 2, (targetH - h) / 2};
}

void logicalToPhysical(int x, int y, uint16_t rotation, int &px, int &py) {
    switch (rotation) {
        case 90:  px = Epd::WIDTH - 1 - y; py = x; break;
        case 180: px = Epd::WIDTH - 1 - x; py = Epd::HEIGHT - 1 - y; break;
        case 270: px = y; py = Epd::HEIGHT - 1 - x; break;
        default:  px = x; py = y; break;
    }
}

void loadIndex() {
    File f = SD.open("/index.txt", FILE_READ);
    if (f) { current = static_cast<uint32_t>(f.parseInt()); f.close(); }
    if (images) current %= images;
    else current = 0;
}
void saveIndex() {
    SD.remove("/index.txt");
    File f = SD.open("/index.txt", FILE_WRITE);
    if (f) { f.println(current); f.flush(); f.close(); }
}

bool showBmp(File &bmp, const Settings &settings, FrameStore &frame) {
    BmpInfo info;
    if (!readBmpHeader(bmp, info)) {
        Serial.println(F("[BMP] Requires uncompressed 24-bit RGB BMP"));
        return false;
    }
    const Fit fit = calculateFit(info.width, info.height, settings.orientation, settings.jpegCover);
    const bool portrait = settings.orientation == 90 || settings.orientation == 270;
    const int targetW = portrait ? Epd::HEIGHT : Epd::WIDTH;
    const int targetH = portrait ? Epd::WIDTH : Epd::HEIGHT;
    Serial.printf("[BMP] %lux%lu -> %dx%d, orientation %u\n",
        (unsigned long)info.width, (unsigned long)info.height,
        fit.width, fit.height, settings.orientation);

    // One BMP source row at a time; optional 4 KB page for oversized rows.
    uint8_t *row = static_cast<uint8_t *>(malloc(info.width * 3U));
    uint8_t page[1024 + 3];
    int32_t prevSourceY = -1;
    uint32_t pageStart = UINT32_MAX;
    uint32_t pageLen = 0;
    for (int y = 0; y < targetH; ++y) {
        if (y < fit.originY || y >= fit.originY + fit.height) continue;
        const uint32_t srcY = min(info.height - 1,
            uint32_t((int64_t(y - fit.originY) * info.height) / fit.height));
        const uint32_t storageY = info.topDown ? srcY : info.height - 1 - srcY;
        const uint32_t offset = info.offset + storageY * info.rowSize;
        if (row && prevSourceY != int32_t(srcY)) {
            if (!bmp.seek(offset) || bmp.read(row, info.width * 3U) != int(info.width * 3U)) {
                free(row);
                return false;
            }
            prevSourceY = int32_t(srcY);
        }
        if (!row && prevSourceY != int32_t(srcY)) {
            prevSourceY = int32_t(srcY);
            pageStart = UINT32_MAX;
        }
        const int minX = max(0, fit.originX);
        const int maxX = min(targetW, fit.originX + fit.width);
        for (int x = minX; x < maxX; ++x) {
            const uint32_t srcX = min(info.width - 1,
                uint32_t((int64_t(x - fit.originX) * info.width) / fit.width));
            const uint32_t byteX = srcX * 3U;
            const uint8_t *p = nullptr;
            if (row) p = row + byteX;
            else {
                if (pageStart == UINT32_MAX || byteX < pageStart ||
                    byteX + 3U > pageStart + pageLen) {
                    pageStart = byteX;
                    const uint32_t length = min(uint32_t(sizeof(page)), info.width * 3U - byteX);
                    if (!bmp.seek(offset + byteX) || bmp.read(page, length) != int(length)) {
                        return false;
                    }
                    pageLen = length;
                }
                p = page + (byteX - pageStart);
            }
            int px, py;
            logicalToPhysical(x, y, settings.orientation, px, py);
            if (!frame.pixel(px, py, Epd::nearest(p[2], p[1], p[0]))) {
                free(row);
                return false;
            }
        }
    }
    free(row);
    return true;
}

struct JpegContext {
    FrameStore *frame = nullptr;
    int sourceWidth = 0;
    int sourceHeight = 0;
    int scaledWidth = 0;
    int scaledHeight = 0;
    int originX = 0;
    int originY = 0;
    uint16_t orientation = 0;
    bool ok = true;
};
JpegContext jpegCtx;

int32_t jpegRead(JPEGFILE *handle, uint8_t *buffer, int32_t length) {
    if (!handle || !handle->fHandle || !buffer || length <= 0) return 0;
    if (handle->iPos < 0 || handle->iPos > handle->iSize) return -1;
    auto *file = static_cast<File *>(handle->fHandle);
    const int32_t remaining = handle->iSize - handle->iPos;
    if (remaining <= 0) return 0;
    const int32_t requested = length < remaining ? length : remaining;
    if (file->position() != uint32_t(handle->iPos) && !file->seek(handle->iPos)) return -1;
    const int32_t received = static_cast<int32_t>(file->read(buffer, requested));
    if (received > 0) handle->iPos += received;
    return received;
}
int32_t jpegSeek(JPEGFILE *handle, int32_t position) {
    if (!handle || !handle->fHandle || position < 0 || position > handle->iSize) return -1;
    auto *file = static_cast<File *>(handle->fHandle);
    if (!file->seek(position)) return -1;
    handle->iPos = position;
    return position;
}

const char *jpegErrorText(int error) {
    switch (error) {
        case JPEG_SUCCESS: return "success";
        case JPEG_INVALID_PARAMETER: return "invalid parameter";
        case JPEG_DECODE_ERROR: return "decode error";
        case JPEG_UNSUPPORTED_FEATURE: return "unsupported JPEG feature";
        case JPEG_INVALID_FILE: return "invalid JPEG data";
        case JPEG_ERROR_MEMORY: return "out of memory";
        default: return "unknown";
    }
}

void printFileSignature(const uint8_t *bytes, size_t count) {
    Serial.print(F("[JPEG] File signature:"));
    for (size_t i = 0; i < count; ++i) Serial.printf(" %02X", bytes[i]);
    Serial.println();
    if (count >= 12 && bytes[0] == 'R' && bytes[1] == 'I' && bytes[2] == 'F' && bytes[3] == 'F' &&
        bytes[8] == 'W' && bytes[9] == 'E' && bytes[10] == 'B' && bytes[11] == 'P') {
        Serial.println(F("[JPEG] File is WebP, despite the .jpg extension"));
    } else if (count >= 8 && bytes[4] == 'f' && bytes[5] == 't' && bytes[6] == 'y' && bytes[7] == 'p') {
        Serial.println(F("[JPEG] File looks like HEIF/AVIF, not JPEG"));
    } else if (count >= 4 && bytes[0] == '<') {
        Serial.println(F("[JPEG] File looks like HTML instead of JPEG"));
    }
}

// Inspect markers without decoding image data. The stream is always rewound.
uint8_t jpegSofMarker(File &file) {
    if (!file.seek(0) || file.read() != 0xFF || file.read() != 0xD8) {
        file.seek(0);
        return 0;
    }
    uint8_t sof = 0;
    // Header parsing is bounded to prevent loops on damaged SD files.
    for (unsigned count = 0; count < 128 && file.position() < file.size(); ++count) {
        int prefix = file.read();
        if (prefix < 0) break;
        if (prefix != 0xFF) continue;
        int marker = file.read();
        while (marker == 0xFF) marker = file.read();
        if (marker < 0 || marker == 0xDA || marker == 0xD9) break;
        if (marker == 0x00 || marker == 0x01 || (marker >= 0xD0 && marker <= 0xD8)) continue;
        int hi = file.read(), lo = file.read();
        if (hi < 0 || lo < 0) break;
        const uint32_t size = (uint32_t(hi) << 8) | uint32_t(lo);
        if (size < 2 || size - 2 > file.size() - file.position()) break;
        if ((marker >= 0xC0 && marker <= 0xCF) &&
            marker != 0xC4 && marker != 0xC8 && marker != 0xCC) {
            sof = uint8_t(marker);
            break;
        }
        if (!file.seek(file.position() + size - 2)) break;
    }
    file.seek(0);
    return sof;
}

int jpegDraw(JPEGDRAW *draw) {
    auto &c = jpegCtx;
    if (!c.ok || !c.frame) return 0;
    const int maxX = min(draw->iWidthUsed > 0 ? draw->iWidthUsed : draw->iWidth,
                         c.sourceWidth - draw->x);
    const int maxY = min(draw->iHeight, c.sourceHeight - draw->y);
    if (maxX <= 0 || maxY <= 0) return 1;
    const bool sideways = c.orientation == 90 || c.orientation == 270;

    // For quarter-turn rotations, walk input columns first to improve SD row-cache locality.
    const int outerLimit = sideways ? maxX : maxY;
    const int innerLimit = sideways ? maxY : maxX;
    for (int outer = 0; outer < outerLimit && c.ok; ++outer) {
        for (int inner = 0; inner < innerLimit && c.ok; ++inner) {
            const int ix = sideways ? outer : inner;
            const int iy = sideways ? inner : outer;
            const int sx = draw->x + ix;
            const int sy = draw->y + iy;
            const uint16_t rgb = draw->pPixels[iy * draw->iWidth + ix];
            const uint8_t r = uint8_t((rgb >> 11) * 255 / 31);
            const uint8_t g = uint8_t(((rgb >> 5) & 0x3F) * 255 / 63);
            const uint8_t b = uint8_t((rgb & 0x1F) * 255 / 31);
            const Epd::Color color = Epd::nearest(r, g, b);
            const int x0 = c.originX + int(int64_t(sx) * c.scaledWidth / c.sourceWidth);
            const int x1 = c.originX + int(int64_t(sx + 1) * c.scaledWidth / c.sourceWidth);
            const int y0 = c.originY + int(int64_t(sy) * c.scaledHeight / c.sourceHeight);
            const int y1 = c.originY + int(int64_t(sy + 1) * c.scaledHeight / c.sourceHeight);
            for (int ly = y0; ly < y1 && c.ok; ++ly) {
                for (int lx = x0; lx < x1; ++lx) {
                    int px, py;
                    logicalToPhysical(lx, ly, c.orientation, px, py);
                    if (!c.frame->pixel(px, py, color)) {
                        c.ok = false;
                        break;
                    }
                }
            }
        }
    }
    return c.ok ? 1 : 0;
}

// An independent baseline decoder handles Huffman tables that JPEGDEC rejects.
// The memory pool is static so a 192KB framebuffer remains available on RP2040.
size_t tinyJpegRead(JDEC *decoder, uint8_t *buffer, size_t requested) {
    auto *source = static_cast<File *>(decoder->device);
    if (!source) return 0;
    if (buffer) return source->read(buffer, requested);
    const uint32_t offset = source->position();
    if (offset > source->size() || requested > source->size() - offset) return 0;
    return source->seek(offset + requested) ? requested : 0;
}

int tinyJpegDraw(JDEC *, void *pixels, JRECT *rect) {
    JPEGDRAW draw = {};
    draw.x = rect->left;
    draw.y = rect->top;
    draw.iWidth = int(rect->right) - int(rect->left) + 1;
    draw.iWidthUsed = draw.iWidth;
    draw.iHeight = int(rect->bottom) - int(rect->top) + 1;
    draw.pPixels = static_cast<uint16_t *>(pixels);
    return jpegDraw(&draw);
}

bool showJpegFallback(File &file, const Settings &settings, FrameStore &frame) {
    static JDEC decoder;
    static uint8_t workspace[TJPGD_WORKSPACE_SIZE + 1024];
    if (!file.seek(0)) return false;
    decoder.swap = 0;
    const JRESULT prepared = jd_prepare(&decoder, tinyJpegRead, workspace,
                                         sizeof(workspace), &file);
    if (prepared != JDR_OK) {
        Serial.printf("[JPEG/TJPG] Header rejected, code %d (progressive or unsupported coding)\n",
                      int(prepared));
        return false;
    }
    const int width = decoder.width;
    const int height = decoder.height;
    const bool portrait = settings.orientation == 90 || settings.orientation == 270;
    const int targetW = portrait ? Epd::HEIGHT : Epd::WIDTH;
    const int targetH = portrait ? Epd::WIDTH : Epd::HEIGHT;
    int shift = 0;
    for (int test = 1; test <= 3; ++test) {
        const int dw = (width + (1 << test) - 1) >> test;
        const int dh = (height + (1 << test) - 1) >> test;
        if (settings.jpegCover ? (dw >= targetW && dh >= targetH) :
                                 (dw >= targetW || dh >= targetH)) shift = test;
        else break;
    }
    const int decodedW = (width + (1 << shift) - 1) >> shift;
    const int decodedH = (height + (1 << shift) - 1) >> shift;
    const Fit fit = calculateFit(decodedW, decodedH, settings.orientation, settings.jpegCover);
    jpegCtx = JpegContext{};
    jpegCtx.frame = &frame;
    jpegCtx.sourceWidth = decodedW;
    jpegCtx.sourceHeight = decodedH;
    jpegCtx.scaledWidth = fit.width;
    jpegCtx.scaledHeight = fit.height;
    jpegCtx.originX = fit.originX;
    jpegCtx.originY = fit.originY;
    jpegCtx.orientation = settings.orientation;
    Serial.printf("[JPEG/TJPG] Baseline fallback: %dx%d -> %dx%d, scale 1/%d\n",
                  width, height, fit.width, fit.height, 1 << shift);
    const JRESULT result = jd_decomp(&decoder, tinyJpegDraw, uint8_t(shift));
    const bool ok = result == JDR_OK && jpegCtx.ok;
    if (!ok) Serial.printf("[JPEG/TJPG] Decode error: %d\n", int(result));
    jpegCtx.frame = nullptr;
    return ok;
}

bool showJpeg(File &file, const Settings &settings, FrameStore &frame) {
    // JPEGDEC state is static to avoid overflowing the RP2040 stack.
    static JPEGDEC jpeg;
    if (file.size() < 256 || file.size() > 0x7FFFFFFFUL || !file.seek(0)) {
        Serial.printf("[JPEG] File too small, too large or not readable (%lu bytes)\n",
                      static_cast<unsigned long>(file.size()));
        return false;
    }
    uint8_t signature[12] = {};
    const int headerRead = file.read(signature, sizeof(signature));
    if (!file.seek(0)) {
        Serial.println(F("[JPEG] Failed to rewind SD file"));
        return false;
    }
    if (headerRead != int(sizeof(signature)) || signature[0] != 0xFF || signature[1] != 0xD8) {
        Serial.println(F("[JPEG] File is not a valid JPEG stream (missing FF D8)"));
        printFileSignature(signature, headerRead > 0 ? size_t(headerRead) : 0);
        return false;
    }
    if (!jpeg.open(static_cast<void *>(&file), static_cast<int>(file.size()),
                   nullptr, jpegRead, jpegSeek, jpegDraw)) {
        const int error = jpeg.getLastError();
        Serial.printf("[JPEG] Could not parse JPEG header: error %d (%s), %lu bytes\n",
                      error, jpegErrorText(error), static_cast<unsigned long>(file.size()));
        printFileSignature(signature, sizeof(signature));
        jpeg.close();
        const uint8_t sof = jpegSofMarker(file);
        if (sof == 0xC0) {
            Serial.println(F("[JPEG] Baseline SOF0: trying independent TJpgDec decoder"));
            return showJpegFallback(file, settings, frame);
        }
        if (sof == 0xC2) {
            Serial.println(F("[JPEG] Progressive SOF2. JPEGDEC rejected the header; "
                             "full decode is unavailable on this firmware."));
            Serial.println(F("[JPEG] Convert with PhotoPainter Studio Web -> Export BMP/EPD"));
        } else {
            Serial.printf("[JPEG] SOF marker: 0x%02X (not supported)\n", sof);
        }
        return false;
    }
    const int imageWidth = jpeg.getWidth();
    const int imageHeight = jpeg.getHeight();
    const bool portrait = settings.orientation == 90 || settings.orientation == 270;
    const int targetW = portrait ? Epd::HEIGHT : Epd::WIDTH;
    const int targetH = portrait ? Epd::WIDTH : Epd::HEIGHT;

    // JPEGDEC only decodes the DC scan of progressive JPEGs (1/8 resolution).
    // This is a compatibility preview, NOT a full-resolution progressive decode.
    const bool progressive = jpeg.getJPEGType() == JPEG_MODE_PROGRESSIVE;
    if (progressive && !settings.progressivePreview) {
        Serial.println(F("[JPEG] Progressive JPEG disabled; re-export as baseline JPEG"));
        jpeg.close();
        return false;
    }
    if (progressive) {
        Serial.println(F("[JPEG] Progressive: 1/8 resolution preview (DC scan only)."));
        Serial.println(F("[JPEG] For full detail use PhotoPainter Studio Web -> Export BMP/EPD"));
    }
    // Prefer the smallest JPEGDEC resolution that can still fill the target.
    int shift = progressive ? 3 : 0;
    for (int test = 1; !progressive && test <= 3; ++test) {
        const int dw = (imageWidth + (1 << test) - 1) >> test;
        const int dh = (imageHeight + (1 << test) - 1) >> test;
        if (settings.jpegCover ? (dw >= targetW && dh >= targetH) :
                                 (dw >= targetW || dh >= targetH)) shift = test;
        else break;
    }
    const int decodedW = (imageWidth + (1 << shift) - 1) >> shift;
    const int decodedH = (imageHeight + (1 << shift) - 1) >> shift;
    const Fit fit = calculateFit(decodedW, decodedH, settings.orientation, settings.jpegCover);
    jpegCtx = JpegContext{};
    jpegCtx.frame = &frame;
    jpegCtx.sourceWidth = decodedW;
    jpegCtx.sourceHeight = decodedH;
    jpegCtx.scaledWidth = fit.width;
    jpegCtx.scaledHeight = fit.height;
    jpegCtx.originX = fit.originX;
    jpegCtx.originY = fit.originY;
    jpegCtx.orientation = settings.orientation;
    jpeg.setPixelType(RGB565_LITTLE_ENDIAN);
    const int options = shift == 1 ? JPEG_SCALE_HALF :
                        shift == 2 ? JPEG_SCALE_QUARTER :
                        shift == 3 ? JPEG_SCALE_EIGHTH : 0;
    Serial.printf("[JPEG] %dx%d -> %dx%d -> %dx%d, orientation %u\n",
                  imageWidth, imageHeight, decodedW, decodedH,
                  fit.width, fit.height, settings.orientation);
    const bool decoded = jpeg.decode(0, 0, options) && jpegCtx.ok;
    if (!decoded) Serial.printf("[JPEG] Decode failed, error %d (%s)\n",
                                jpeg.getLastError(), jpegErrorText(jpeg.getLastError()));
    jpeg.close();
    jpegCtx.frame = nullptr;
    return decoded;
}

}

namespace Gallery {
uint32_t scanDirectory(const char *directory) {
    File dir = SD.open(directory);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        Serial.printf("[GALLERY] Directory not found: %s\n", directory);
        return 0;
    }
    sortedNames.clear();
    while (File entry = dir.openNextFile()) {
        if (!entry.isDirectory() && isImage(String(entry.name()))) {
            if (sortedNames.size() < 512) sortedNames.push_back(baseName(String(entry.name())));
            else { entry.close(); Serial.println(F("[GALLERY] Limit 512 images reached")); break; }
        }
        entry.close();
    }
    dir.close();
    std::sort(sortedNames.begin(), sortedNames.end(), [](const String &a, const String &b) {
        return a.compareTo(b) < 0;
    });
    Serial.printf("[GALLERY] %s: %lu supported images (sorted)\n", directory,
                  (unsigned long)sortedNames.size());
    return sortedNames.size();
}

bool begin() {
    if (!mounted) {
        SPI.setSCK(Board::SD_SCK);
        SPI.setTX(Board::SD_MOSI);
        SPI.setRX(Board::SD_MISO);
        SPI.begin();
        mounted = SD.begin(Board::SD_CS);
        if (!mounted) Serial.println(F("[SD] Mount failed. Check wiring/FAT32."));
    }
    if (mounted) reload();
    return mounted;
}
bool reload() {
    if (!mounted) return false;
    images = scanDirectory("/pic");
    activeDirectory = "/pic";
    // Compatible with FAT cards where directory was named using another case.
    if (!images) {
        const char *alternates[] = {"/PIC", "/Pic"};
        for (const char *alternate : alternates) {
            const uint32_t found = scanDirectory(alternate);
            if (found) { images = found; activeDirectory = alternate; break; }
        }
    }
    // Root fallback helps when images were copied there by mistake.
    if (!images) {
        const uint32_t found = scanDirectory("/");
        if (found) {
            images = found;
            activeDirectory = "/";
            Serial.println(F("[GALLERY] Using root JPG/BMP files; /pic remains recommended"));
        }
    }
    if (!images) Serial.println(F("[GALLERY] No images found. Send SDINFO to inspect the card."));
    loadIndex();
    loadShuffleStep();
    return true;
}
uint32_t count() { return images; }
uint32_t nextIndex() { return current; }
void setNextIndex(uint32_t index) {
    const uint32_t next = images ? index % images : 0;
    if (next == current) return; // Avoid unnecessary SD writes.
    current = next;
    saveIndex();
}
uint32_t nextShuffleIndex(uint32_t index) {
    if (images < 2) return 0;
    return (index + shuffleStep) % images;
}
bool choose(String &path, uint32_t &index) {
    if (!mounted || !images || sortedNames.empty()) return false;
    index = current % images;
    path = activeDirectory == "/" ? String("/") + sortedNames[index] :
           activeDirectory + "/" + sortedNames[index];
    return true;
}

bool show(const String &path, const Settings &settings) {
    if (!mounted) return false;
    File image = SD.open(path.c_str(), FILE_READ);
    if (!image) return false;
    const FrameCache::Identity id = FrameCache::identify(path, image, settings);
    if (settings.skipUnchanged && FrameCache::alreadyDisplayed(id)) {
        Serial.println(F("[POWER] Identical image already on EPD: skip full refresh"));
        image.close();
        return true;
    }
    if (NativeFrame::isNative(path)) {
        // EPD/EDP already has its final physical orientation and 7-color palette.
        const bool success = NativeFrame::display(image);
        image.close();
        if (success) FrameCache::markDisplayed(id);
        return success;
    }
    if (settings.cacheEnabled) {
        const FrameCache::LoadResult result = FrameCache::loadAndDisplay(id);
        if (result == FrameCache::LoadResult::Displayed) {
            FrameCache::markDisplayed(id);
            image.close();
            return true;
        }
        if (result == FrameCache::LoadResult::DisplayError) {
            image.close();
            Serial.println(F("[CACHE] EPD error: abort instead of costly reconversion"));
            return false;
        }
    }
    static FrameStore frame;
    if (!frame.begin()) { image.close(); return false; }
    const bool converted = isJpeg(path) ? showJpeg(image, settings, frame) :
                                           showBmp(image, settings, frame);
    image.close();
    if (!converted) {
        frame.close();
        return false;
    }
    // A complete 192 KB cache is written only once per source/settings change.
    // Skip extra SD traffic close to low-battery cutoff.
    if (settings.cacheEnabled &&
        (Power::usbPresent() || Power::batteryVoltage() >= settings.cacheWriteMinV)) {
        if (!FrameCache::write(id, frame)) Serial.println(F("[CACHE] Save failed; rendering anyway"));
    } else if (settings.cacheEnabled) Serial.println(F("[CACHE] Skipping write: low battery"));

    const bool success = frame.render();
    frame.close();
    if (success) FrameCache::markDisplayed(id);
    return success;
}

void printDirectory(Stream &out, const char *path) {
    File dir = SD.open(path);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        out.printf("[SD] %s: missing or not a directory\n", path);
        return;
    }
    out.printf("[SD] Directory %s:\n", path);
    uint32_t total = 0;
    while (File entry = dir.openNextFile()) {
        const String name = String(entry.name());
        out.printf("  %s%s (%lu bytes)%s\n", name.c_str(),
                   entry.isDirectory() ? "/" : "", (unsigned long)entry.size(),
                   (!entry.isDirectory() && isImage(name)) ? " [IMAGE]" : "");
        ++total;
        entry.close();
        if (total >= 100) { out.println(F("  ... first 100 entries shown")); break; }
    }
    dir.close();
    if (!total) out.println(F("  (empty)"));
}
void sdInfo(Stream &out) {
    if (!mounted) { out.println(F("[SD] Mount failed")); return; }
    printDirectory(out, "/");
    printDirectory(out, "/pic");
    if (activeDirectory != "/" && activeDirectory != "/pic")
        printDirectory(out, activeDirectory.c_str());
    out.printf("[SD] Selected image directory: %s, count: %lu\n",
               activeDirectory.c_str(), (unsigned long)images);
}
void list(Stream &out) {
    if (!mounted) { out.println(F("SD unavailable")); return; }
    out.printf("[GALLERY] %lu image(s) in %s (alphabetical order)\n", (unsigned long)images, activeDirectory.c_str());
    for (size_t i = 0; i < sortedNames.size(); ++i) {
        out.printf("%lu: %s\n", (unsigned long)i, sortedNames[i].c_str());
    }
}

int16_t parseClock(const String &value) {
    if (value.length() != 5 || value[2] != ':') return -1;
    if (!isDigit(value[0]) || !isDigit(value[1]) ||
        !isDigit(value[3]) || !isDigit(value[4])) return -1;
    const int hours = (value[0] - '0') * 10 + value[1] - '0';
    const int minutes = (value[3] - '0') * 10 + value[4] - '0';
    return hours < 24 && minutes < 60 ? hours * 60 + minutes : -1;
}
void loadSettings(Settings &settings) {
    if (!mounted) return;
    File f = SD.open("/settings.conf", FILE_READ);
    if (!f) {
        f = SD.open("/frame.ini", FILE_READ);
        if (!f) { Serial.println(F("[CONFIG] Defaults in use")); return; }
        Serial.println(F("[CONFIG] Legacy frame.ini (rename to settings.conf)"));
    }
    while (f.available()) {
        String line = f.readStringUntil('\n');
        line.trim();
        if (!line.length() || line.startsWith("#") || line.startsWith(";")) continue;
        const int sep = line.indexOf('=');
        if (sep < 1) continue;
        String key = line.substring(0, sep), value = line.substring(sep + 1);
        key.trim(); value.trim(); key.toLowerCase(); value.toLowerCase();
        if (key == "refresh_minutes" || key == "interval_minutes") {
            const long v = value.toInt();
            if (v >= 1 && v <= 10080) settings.refreshMinutes = v;
        } else if (key == "orientation") {
            const int v = value.toInt();
            if ((v == 0 || v == 90 || v == 180 || v == 270) && value == String(v))
                settings.orientation = v;
        } else if (key == "quiet_hours_start") {
            const int v = parseClock(value);
            if (v >= 0) settings.quietStart = uint16_t(v);
        } else if (key == "quiet_hours_end") {
            const int v = parseClock(value);
            if (v >= 0) settings.quietEnd = uint16_t(v);
        } else if (key == "battery_calibration") {
            const float v = value.toFloat();
            if (v >= .85f && v <= 1.15f) settings.batteryCalibration = v;
        } else if (key == "shuffle") {
            settings.shuffle = value == "true" || value == "1" || value == "on";
        } else if (key == "progressive_jpeg") {
            if (value == "preview" || value == "reject")
                settings.progressivePreview = value == "preview";
        } else if (key == "jpeg_fit" || key == "image_fit") {
            if (value == "cover" || value == "contain") settings.jpegCover = value == "cover";
        } else if (key == "cache_enabled") {
            settings.cacheEnabled = value == "true" || value == "1" || value == "on";
        } else if (key == "skip_unchanged") {
            settings.skipUnchanged = value == "true" || value == "1" || value == "on";
        } else if (key == "single_image_auto_pause") {
            settings.singleImageAutoPause = value == "true" || value == "1" || value == "on";
        } else if (key == "activity_led") {
            settings.activityLed = value == "true" || value == "1" || value == "on";
        } else if (key == "usb_auto_next") {
            settings.usbAutoNext = value == "true" || value == "1" || value == "on";
        } else if (key == "cache_write_min_v") {
            const float v = value.toFloat();
            if (v >= 3.3f && v <= 4.2f) settings.cacheWriteMinV = v;
        } else if (key == "low_battery_v") {
            const float v = value.toFloat();
            if (v >= 3.2f && v <= 3.8f) settings.lowBatteryV = v;
        } else if (key == "critical_cutoff_v") {
            const float v = value.toFloat();
            if (v >= 3.0f && v <= 3.4f) settings.criticalCutoffV = v;
        }
    }
    f.close();
    if (settings.criticalCutoffV >= settings.lowBatteryV)
        settings.criticalCutoffV = settings.lowBatteryV - 0.1f;
    if (settings.cacheWriteMinV < settings.lowBatteryV + 0.1f)
        settings.cacheWriteMinV = settings.lowBatteryV + 0.1f;
}
}
