#pragma once
#include <Arduino.h>

struct DateTime {
    uint16_t year = 2000;
    uint8_t month = 1, day = 1, hour = 0, minute = 0, second = 0;
};

namespace Rtc {
bool begin();
bool now(DateTime &date);
bool set(const DateTime &date);
bool scheduleAfter(uint16_t minutes);
bool scheduleAfterQuiet(uint16_t minutes, uint16_t quietStart, uint16_t quietEnd);
bool alarmFired();
bool clearAlarm();
bool disableAlarm();
bool isValid(const DateTime &date);
}
