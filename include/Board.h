#pragma once

#include <Arduino.h>

namespace Board {
constexpr uint8_t EPD_RST = 12;
constexpr uint8_t EPD_DC = 8;
constexpr uint8_t EPD_CS = 9;
constexpr uint8_t EPD_BUSY = 13;
constexpr uint8_t EPD_SCK = 10;
constexpr uint8_t EPD_MOSI = 11;
constexpr uint8_t EPD_POWER = 16;

constexpr uint8_t SD_CS = 5;
constexpr uint8_t SD_SCK = 2;
constexpr uint8_t SD_MOSI = 3;
constexpr uint8_t SD_MISO = 4;

constexpr uint8_t RTC_SDA = 14;
constexpr uint8_t RTC_SCL = 15;
constexpr uint8_t RTC_INT = 6;

constexpr uint8_t BAT_CUT = 18;
constexpr uint8_t NEXT_BUTTON = 19;
constexpr uint8_t CHARGE_STATE = 17;
constexpr uint8_t VBUS_SENSE = 24;
constexpr uint8_t BAT_ADC = 29;
constexpr uint8_t LED_ACTIVITY = 25;
constexpr uint8_t LED_POWER = 26;
}

namespace Power {
void begin();
float batteryVoltage();
void setBatteryCalibration(float factor);
bool usbPresent();
bool charging();
void blink(uint8_t pin, int count, uint16_t durationMs = 130);
void cutPower();
}
