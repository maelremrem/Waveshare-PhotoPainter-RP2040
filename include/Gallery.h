#pragma once
#include <Arduino.h>

struct Settings {
    uint16_t refreshMinutes = 720;
    uint16_t quietStart = 0; // Minutes since midnight. Equal start/end disables quiet hours.
    uint16_t quietEnd = 0;
    float batteryCalibration = 1.0f;
    uint16_t orientation = 0;
    bool shuffle = false;
    bool jpegCover = false;
    bool progressivePreview = true; // JPEGDEC supports only 1/8-size DC preview.
    bool cacheEnabled = true;
    bool skipUnchanged = true;
    bool singleImageAutoPause = true;
    bool activityLed = true;
    bool usbAutoNext = false; // Physical AUTO-NEXT switch is a hardware gate, not a GPIO input.
    float cacheWriteMinV = 3.6f;
    float lowBatteryV = 3.4f;
    float criticalCutoffV = 3.1f;
};

namespace Gallery {
bool begin();
bool reload();
uint32_t count();
uint32_t nextIndex();
void setNextIndex(uint32_t index);
bool choose(String &path, uint32_t &index);
uint32_t nextShuffleIndex(uint32_t index);
bool show(const String &path, const Settings &settings);
void loadSettings(Settings &settings);
void list(Stream &out);
void sdInfo(Stream &out);
}
