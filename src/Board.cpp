#include "Board.h"

namespace { float batteryCalibration = 1.0f; }
namespace Power {
void setBatteryCalibration(float factor) { if (factor >= 0.85f && factor <= 1.15f) batteryCalibration = factor; }
void begin() {
    pinMode(Board::BAT_CUT, OUTPUT);
    digitalWrite(Board::BAT_CUT, HIGH);
    pinMode(Board::EPD_POWER, OUTPUT);
    digitalWrite(Board::EPD_POWER, LOW);
    pinMode(Board::NEXT_BUTTON, INPUT_PULLUP);
    pinMode(Board::RTC_INT, INPUT_PULLUP);
    pinMode(Board::CHARGE_STATE, INPUT_PULLUP);
    pinMode(Board::VBUS_SENSE, INPUT);
    pinMode(Board::LED_ACTIVITY, OUTPUT);
    pinMode(Board::LED_POWER, OUTPUT);
    digitalWrite(Board::LED_ACTIVITY, LOW);
    digitalWrite(Board::LED_POWER, LOW);
    analogReadResolution(12);
}

bool usbPresent() { return digitalRead(Board::VBUS_SENSE) == HIGH; }
bool charging() { return usbPresent() && digitalRead(Board::CHARGE_STATE) == LOW; }

float batteryVoltage() {
    uint32_t total = 0;
    for (uint8_t i = 0; i < 16; ++i) {
        total += analogRead(Board::BAT_ADC);
        delay(2);
    }
    return (static_cast<float>(total) / 16.0f) * (3.3f / 4095.0f) * 3.0f * batteryCalibration;
}

void blink(uint8_t pin, int count, uint16_t durationMs) {
    for (int i = 0; i < count; ++i) {
        digitalWrite(pin, HIGH);
        delay(durationMs);
        digitalWrite(pin, LOW);
        delay(durationMs);
    }
}

void cutPower() {
    digitalWrite(Board::EPD_POWER, LOW);
    digitalWrite(Board::LED_ACTIVITY, LOW);
    digitalWrite(Board::LED_POWER, LOW);
    delay(20);
    digitalWrite(Board::BAT_CUT, LOW);
}
}
