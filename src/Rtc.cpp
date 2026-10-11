#include "Rtc.h"
#include "Board.h"
#include <Wire.h>

namespace {
constexpr uint8_t ADDR = 0x51;
constexpr uint8_t CTRL2 = 0x01;
constexpr uint8_t REG_SECONDS = 0x04;
constexpr uint8_t REG_ALARM_SECONDS = 0x0B;

uint8_t bcd(uint8_t value) { return (value / 10U) * 16U + value % 10U; }
uint8_t dec(uint8_t value) { return (value >> 4U) * 10U + (value & 0x0F); }

bool writeReg(uint8_t reg, uint8_t value) {
    Wire1.beginTransmission(ADDR);
    Wire1.write(reg);
    Wire1.write(value);
    return Wire1.endTransmission() == 0;
}
bool readRegs(uint8_t reg, uint8_t *values, uint8_t count) {
    Wire1.beginTransmission(ADDR);
    Wire1.write(reg);
    if (Wire1.endTransmission(false) != 0) return false;
    if (Wire1.requestFrom(ADDR, count) != count) return false;
    for (uint8_t i = 0; i < count; ++i) values[i] = Wire1.read();
    return true;
}
bool readReg(uint8_t reg, uint8_t &value) { return readRegs(reg, &value, 1); }
uint8_t daysInMonth(uint16_t year, uint8_t month) {
    static constexpr uint8_t days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (month < 1 || month > 12) return 0;
    if (month == 2 && ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)) return 29;
    return days[month - 1];
}
void addOneDay(DateTime &d) {
    if (++d.day <= daysInMonth(d.year, d.month)) return;
    d.day = 1;
    if (++d.month <= 12) return;
    d.month = 1;
    ++d.year;
}
}

namespace Rtc {
bool begin() {
    Wire1.setSDA(Board::RTC_SDA);
    Wire1.setSCL(Board::RTC_SCL);
    Wire1.begin();
    Wire1.setClock(100000);
    Wire1.beginTransmission(ADDR);
    return Wire1.endTransmission() == 0;
}
bool isValid(const DateTime &d) {
    return d.year >= 2000 && d.year <= 2099 && d.month >= 1 && d.month <= 12 &&
           d.day >= 1 && d.day <= daysInMonth(d.year, d.month) &&
           d.hour < 24 && d.minute < 60 && d.second < 60;
}
bool now(DateTime &d) {
    uint8_t v[7];
    if (!readRegs(REG_SECONDS, v, sizeof(v))) return false;
    d.second = dec(v[0] & 0x7F);
    d.minute = dec(v[1] & 0x7F);
    d.hour = dec(v[2] & 0x3F);
    d.day = dec(v[3] & 0x3F);
    d.month = dec(v[5] & 0x1F);
    d.year = 2000 + dec(v[6]);
    return isValid(d);
}
bool set(const DateTime &d) {
    if (!isValid(d)) return false;
    bool ok = true;
    ok &= writeReg(0x0A, bcd(d.year - 2000));
    ok &= writeReg(0x09, bcd(d.month));
    ok &= writeReg(0x07, bcd(d.day));
    ok &= writeReg(0x06, bcd(d.hour));
    ok &= writeReg(0x05, bcd(d.minute));
    ok &= writeReg(0x04, bcd(d.second));
    return ok;
}
bool clearAlarm() {
    uint8_t ctrl;
    if (!readReg(CTRL2, ctrl)) return false;
    return writeReg(CTRL2, ctrl & ~0x40U);
}
bool alarmFired() {
    uint8_t ctrl;
    return readReg(CTRL2, ctrl) && (ctrl & 0x40U);
}
bool disableAlarm() {
    uint8_t ctrl;
    if (!readReg(CTRL2, ctrl)) return false;
    bool ok = writeReg(CTRL2, ctrl & ~0x80U);
    for (uint8_t reg = REG_ALARM_SECONDS; reg <= 0x0E; ++reg) ok &= writeReg(reg, 0x80);
    return ok;
}
bool scheduleAfter(uint16_t minutes) { return scheduleAfterQuiet(minutes, 0, 0); }
bool scheduleAfterQuiet(uint16_t minutes, uint16_t quietStart, uint16_t quietEnd) {
    if (minutes < 1 || minutes > 10080) return false;
    DateTime d;
    if (!now(d)) return false;
    uint32_t total = static_cast<uint32_t>(d.hour) * 3600UL + d.minute * 60UL + d.second;
    total += static_cast<uint32_t>(minutes) * 60UL;
    while (total >= 86400UL) {
        total -= 86400UL;
        addOneDay(d);
    }
    d.hour = total / 3600UL;
    d.minute = (total % 3600UL) / 60UL;
    d.second = total % 60UL;
    if (quietStart != quietEnd && quietStart < 1440 && quietEnd < 1440) {
        const uint16_t minuteOfDay = uint16_t(d.hour) * 60 + d.minute;
        const bool quiet = quietStart < quietEnd ?
            (minuteOfDay >= quietStart && minuteOfDay < quietEnd) :
            (minuteOfDay >= quietStart || minuteOfDay < quietEnd);
        if (quiet) {
            // Align to the next quiet-end boundary, even across midnight.
            if (minuteOfDay >= quietEnd) addOneDay(d);
            d.hour = quietEnd / 60;
            d.minute = quietEnd % 60;
            d.second = 0;
        }
    }
    if (!clearAlarm()) return false;
    bool ok = true;
    ok &= writeReg(0x0B, bcd(d.second) & 0x7F);
    ok &= writeReg(0x0C, bcd(d.minute) & 0x7F);
    ok &= writeReg(0x0D, bcd(d.hour) & 0x3F);
    ok &= writeReg(0x0E, bcd(d.day) & 0x3F);
    ok &= writeReg(0x0F, 0x80); // Disable weekday matching
    uint8_t ctrl;
    if (!readReg(CTRL2, ctrl)) return false;
    return ok && writeReg(CTRL2, (ctrl & ~0x40U) | 0x80U);
}
}
