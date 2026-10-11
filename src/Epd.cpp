/*
 * Display controller initialization adapted from:
 * Waveshare team - EPD_7in3f.c, V1.0, 2022-08-04.
 * Original source: https://github.com/waveshareteam/PhotoPainter/blob/master/lib/e-Paper/EPD_7in3f.c
 * Original permission notice (retained as required):
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documnetation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "Epd.h"
#include "Board.h"
#include <SPI.h>
#include <initializer_list>

namespace {
SPISettings epdSpeed(8000000, MSBFIRST, SPI_MODE0);

void command(uint8_t v) {
    digitalWrite(Board::EPD_DC, LOW);
    SPI1.beginTransaction(epdSpeed);
    digitalWrite(Board::EPD_CS, LOW);
    SPI1.transfer(v);
    digitalWrite(Board::EPD_CS, HIGH);
    SPI1.endTransaction();
}
void data(uint8_t v) {
    digitalWrite(Board::EPD_DC, HIGH);
    SPI1.beginTransaction(epdSpeed);
    digitalWrite(Board::EPD_CS, LOW);
    SPI1.transfer(v);
    digitalWrite(Board::EPD_CS, HIGH);
    SPI1.endTransaction();
}
void reset() {
    digitalWrite(Board::EPD_RST, HIGH); delay(20);
    digitalWrite(Board::EPD_RST, LOW); delay(5);
    digitalWrite(Board::EPD_RST, HIGH); delay(20);
}
bool waitReady(uint32_t timeoutMs = 70000) {
    const uint32_t start = millis();
    while (digitalRead(Board::EPD_BUSY) == LOW) {
        if (millis() - start > timeoutMs) {
            Serial.println(F("[EPD] BUSY timeout"));
            return false;
        }
        delay(10);
    }
    return true;
}
void cmdData(uint8_t cmd, std::initializer_list<uint8_t> values) {
    command(cmd);
    for (auto v : values) data(v);
}
}

namespace Epd {
bool begin() {
    pinMode(Board::EPD_RST, OUTPUT);
    pinMode(Board::EPD_DC, OUTPUT);
    pinMode(Board::EPD_CS, OUTPUT);
    pinMode(Board::EPD_BUSY, INPUT);
    digitalWrite(Board::EPD_CS, HIGH);
    digitalWrite(Board::EPD_POWER, HIGH);
    delay(50);
    SPI1.setSCK(Board::EPD_SCK);
    SPI1.setTX(Board::EPD_MOSI);
    SPI1.begin();
    reset();
    if (!waitReady(8000)) return false;
    delay(30);
    cmdData(0xAA, {0x49,0x55,0x20,0x08,0x09,0x18});
    cmdData(0x01, {0x3F,0x00,0x32,0x2A,0x0E,0x2A});
    cmdData(0x00, {0x5F,0x69});
    cmdData(0x03, {0x00,0x54,0x00,0x44});
    cmdData(0x05, {0x40,0x1F,0x1F,0x2C});
    cmdData(0x06, {0x6F,0x1F,0x1F,0x22});
    cmdData(0x08, {0x6F,0x1F,0x1F,0x22});
    cmdData(0x13, {0x00,0x04});
    cmdData(0x30, {0x3C});
    cmdData(0x41, {0x00});
    cmdData(0x50, {0x3F});
    cmdData(0x60, {0x02,0x00});
    cmdData(0x61, {0x03,0x20,0x01,0xE0});
    cmdData(0x82, {0x1E});
    cmdData(0x84, {0x00});
    cmdData(0x86, {0x00});
    cmdData(0xE3, {0x2F});
    cmdData(0xE0, {0x00});
    cmdData(0xE6, {0x00});
    return true;
}
void startFrame() {
    command(0x10);
    digitalWrite(Board::EPD_DC, HIGH);
    SPI1.beginTransaction(epdSpeed);
    digitalWrite(Board::EPD_CS, LOW);
}
void writeRow(const uint8_t *pixels, size_t count) {
    for (size_t i = 0; i < count; ++i) SPI1.transfer(pixels[i]);
}
void endFrame() {
    digitalWrite(Board::EPD_CS, HIGH);
    SPI1.endTransaction();
}
bool refresh() {
    command(0x04);
    delay(25);
    if (!waitReady()) return false;
    cmdData(0x12, {0x00});
    delay(100);
    if (!waitReady()) return false;
    cmdData(0x02, {0x00});
    delay(20);
    return waitReady();
}
void sleep() {
    cmdData(0x07, {0xA5});
    digitalWrite(Board::EPD_POWER, LOW);
}
bool clear(Color color) {
    if (!begin()) { sleep(); return false; }
    uint8_t packed[WIDTH/2];
    memset(packed, static_cast<uint8_t>(color) * 0x11, sizeof(packed));
    startFrame();
    for (int y = 0; y < HEIGHT; ++y) writeRow(packed, sizeof(packed));
    endFrame();
    const bool success = refresh();
    sleep();
    return success;
}
bool testPattern() {
    if (!begin()) { sleep(); return false; }
    uint8_t row[WIDTH/2];
    startFrame();
    for (int y = 0; y < HEIGHT; ++y) {
        for (int x = 0; x < WIDTH; x += 2) {
            uint8_t left = min(6, x / 115);
            uint8_t right = min(6, (x + 1) / 115);
            row[x/2] = (left << 4) | right;
        }
        writeRow(row, sizeof(row));
    }
    endFrame();
    const bool success = refresh();
    sleep();
    return success;
}
bool showLowBattery(uint16_t orientation) {
    if (!begin()) { sleep(); return false; }
    uint8_t row[WIDTH / 2];
    const bool portrait = orientation == 90 || orientation == 270;
    const int w = portrait ? HEIGHT : WIDTH;
    const int h = portrait ? WIDTH : HEIGHT;
    startFrame();
    for (int y = 0; y < HEIGHT; ++y) {
        auto colorAt = [y, orientation, w, h](int x) -> uint8_t {
            int lx = x, ly = y;
            switch (orientation) {
                case 90:  lx = y; ly = WIDTH - 1 - x; break;
                case 180: lx = WIDTH - 1 - x; ly = HEIGHT - 1 - y; break;
                case 270: lx = HEIGHT - 1 - y; ly = x; break;
                default: break;
            }
            const bool body = lx >= w * 13 / 100 && lx < w * 79 / 100 &&
                              ly >= h * 19 / 100 && ly < h * 81 / 100;
            const bool rim = body &&
                (lx < w * 17 / 100 || lx >= w * 75 / 100 ||
                 ly < h * 25 / 100 || ly >= h * 75 / 100);
            const bool terminal = lx >= w * 79 / 100 && lx < w * 88 / 100 &&
                                  ly >= h * 37 / 100 && ly < h * 63 / 100;
            uint8_t color = (rim || terminal) ? BLACK : WHITE;
            const int slashStartY = h * 10 / 100;
            const int slashEndY = h * 90 / 100;
            if (ly >= slashStartY && ly <= slashEndY) {
                const int slashX = w * 12 / 100 +
                    (ly - slashStartY) * (w * 76 / 100) / (slashEndY - slashStartY);
                if (abs(lx - slashX) < max(9, w * 3 / 100)) color = RED;
            }
            return color;
        };
        for (int x = 0; x < WIDTH; x += 2)
            row[x / 2] = uint8_t((colorAt(x) << 4) | colorAt(x + 1));
        writeRow(row, sizeof(row));
    }
    endFrame();
    const bool ok = refresh();
    sleep();
    return ok;
}

Color nearest(uint8_t r, uint8_t g, uint8_t b) {
    static constexpr uint8_t rgb[7][3] = {
        {0,0,0}, {255,255,255}, {0,255,0}, {0,0,255},
        {255,0,0}, {255,255,0}, {255,128,0}
    };
    uint32_t best = UINT32_MAX;
    uint8_t chosen = 1;
    for (uint8_t i = 0; i < 7; ++i) {
        const int dr = int(r) - rgb[i][0];
        const int dg = int(g) - rgb[i][1];
        const int db = int(b) - rgb[i][2];
        const uint32_t err = static_cast<uint32_t>(3 * dr * dr + 4 * dg * dg + 2 * db * db);
        if (err < best) { best = err; chosen = i; }
    }
    return static_cast<Color>(chosen);
}
}
