#include <Arduino.h>
#include <SD.h>
#include <pico/bootrom.h>
#include "Board.h"
#include "Rtc.h"
#include "Epd.h"
#include "Gallery.h"
#include "FrameCache.h"

namespace {
Settings settings;
bool sdOK = false;
bool rtcOK = false;
bool imageShown = false;
uint32_t lastButtonChange = 0;
uint32_t lastStatusPoll = 0;
bool buttonStable = true;
bool buttonLastRaw = true;
String commandLine;

void printDate(Stream &out, const DateTime &d) {
    out.printf("%04u-%02u-%02u %02u:%02u:%02u\n",
        d.year, d.month, d.day, d.hour, d.minute, d.second);
}

void status() {
    Serial.printf("PhotoPainter RP2040 firmware %s\n", FW_VERSION);
    Serial.printf("USB: %s | charging: %s | battery: ",
        Power::usbPresent() ? "yes" : "no", Power::charging() ? "yes" : "no");
    Serial.print(Power::batteryVoltage(), 2);
    Serial.println(F(" V"));
    Serial.printf("SD: %s | images: %lu | next: %lu | shuffle: %s | interval: %u min\n",
        sdOK ? "ok" : "missing", (unsigned long)Gallery::count(),
        (unsigned long)Gallery::nextIndex(), settings.shuffle ? "on" : "off", settings.refreshMinutes);
    Serial.printf("Orientation: %u deg | JPEG: %s | progressive: %s | low: ",
        settings.orientation, settings.jpegCover ? "cover" : "contain",
        settings.progressivePreview ? "preview" : "reject");
    Serial.print(settings.lowBatteryV, 2);
    Serial.print(F(" V | critical: "));
    Serial.print(settings.criticalCutoffV, 2);
    Serial.println(F(" V"));
    Serial.printf("Cache: %s | skip unchanged: %s | 1-photo pause: %s | save above: ",
        settings.cacheEnabled ? "on" : "off", settings.skipUnchanged ? "on" : "off",
        settings.singleImageAutoPause ? "on" : "off");
    Serial.print(settings.cacheWriteMinV, 2);
    Serial.println(F(" V"));
    Serial.printf("LED ACT: %s | RTC on USB: %s | AUTO-NEXT: hardware RTC gate\n",
        settings.activityLed ? "on" : "off", settings.usbAutoNext ? "on" : "off");
    Serial.printf("Quiet hours: %02u:%02u-%02u:%02u | calibration: ",
        settings.quietStart/60, settings.quietStart%60, settings.quietEnd/60, settings.quietEnd%60);
    Serial.print(settings.batteryCalibration, 3); Serial.println();
    DateTime d;
    Serial.print(F("RTC: "));
    if (rtcOK && Rtc::now(d)) printDate(Serial, d);
    else Serial.println(F("not detected or time invalid"));
}

bool displayNext() {
    if (!sdOK || Gallery::count() == 0) {
        Serial.println(F("[GALLERY] No usable images. Check SD FAT32 and /pic/*.epd|bmp|jpg|jpeg"));
        return false;
    }
    String path;
    uint32_t index = 0;
    if (!Gallery::choose(path, index)) return false;
    Serial.printf("[GALLERY] Showing %lu: %s\n", (unsigned long)index, path.c_str());
    // Keep ACT visible for the entire SD read, JPEG/BMP conversion, and EPD refresh.
    // On battery this can be disabled from settings.conf to conserve power.
    if (settings.activityLed) digitalWrite(Board::LED_ACTIVITY, HIGH);
    const bool success = Gallery::show(path, settings);
    digitalWrite(Board::LED_ACTIVITY, LOW);
    if (!success) {
        // A corrupt or unsupported image must not trap NEXT on the same item forever.
        if (Gallery::count() > 1) {
            Gallery::setNextIndex((index + 1U) % Gallery::count());
            Serial.println(F("[GALLERY] Render failed. Skipping this file on next attempt."));
        } else {
            Serial.println(F("[GALLERY] Render failed. No other image to try."));
        }
        return false;
    }
    imageShown = true;
    uint32_t next = settings.shuffle ? Gallery::nextShuffleIndex(index) : (index + 1U) % Gallery::count();
    Gallery::setNextIndex(next);
    Serial.println(F("[GALLERY] Image processed"));
    return true;
}

void scheduleWake() {
    if (!rtcOK) return;
    if (settings.singleImageAutoPause && imageShown && Gallery::count() == 1) {
        Rtc::disableAlarm();
        Serial.println(F("[POWER] Single picture displayed: RTC auto-wake paused"));
        return;
    }
    if (!Gallery::count()) {
        Rtc::disableAlarm();
        Serial.println(F("[POWER] No images: RTC auto-wake paused"));
        return;
    }
    if (!Rtc::scheduleAfterQuiet(settings.refreshMinutes, settings.quietStart, settings.quietEnd)) {
        Serial.println(F("[RTC] Cannot schedule wake. Set time over USB with SETTIME."));
    } else {
        Serial.printf("[RTC] Next alarm (quiet hours adjusted): %u minute interval\n", settings.refreshMinutes);
    }
}

void finishBatteryCycle() {
    if (rtcOK) { Rtc::clearAlarm(); scheduleWake(); }
    Serial.flush();
    Power::cutPower();
    while (true) delay(1000);
}

void help() {
    Serial.println(F("HELP                Show commands"));
    Serial.println(F("STATUS              Board diagnostics"));
    Serial.println(F("LIST                List EPD/BMP/JPEG gallery files"));
    Serial.println(F("SDINFO              Show SD root and /pic (debug images: 0)"));
    Serial.println(F("CACHE STATS         Cached image count and bytes"));
    Serial.println(F("CACHE CLEAR         Remove converted pictures and shown marker"));
    Serial.println(F("CACHE ON|OFF        Toggle cache in RAM only"));
    Serial.println(F("RESCAN              Reinitialize SD and count files"));
    Serial.println(F("NEXT                Show next image"));
    Serial.println(F("SHOW n              Show image at index n"));
    Serial.println(F("TEST                Draw 7-color test pattern"));
    Serial.println(F("CLEAR               Clear screen to white"));
    Serial.println(F("LOWBAT              Preview low-battery icon (USB only)"));
    Serial.println(F("TIME                Read RTC"));
    Serial.println(F("SETTIME YYYY-MM-DD HH:MM:SS"));
    Serial.println(F("INTERVAL n          RTC alarm every 1..10080 minutes (RAM only)"));
    Serial.println(F("LEDACT ON|OFF       Activity indicator (RAM only)"));
    Serial.println(F("USB AUTO ON|OFF     RTC slideshow while USB attached (RAM only)"));
    Serial.println(F("ORIENTATION n       0, 90, 180, 270 (RAM only)"));
    Serial.println(F("SHUFFLE ON|OFF      Shuffle next image (RAM only)"));
    Serial.println(F("SLEEP               Schedule wake and cut battery power"));
    Serial.println(F("BOOTLOADER          Enter UF2 flashing mode"));
}

void runCommand(String line) {
    line.trim();
    if (!line.length()) return;
    String upper = line;
    upper.toUpperCase();
    if (upper == "HELP" || upper == "?") help();
    else if (upper == "STATUS") status();
    else if (upper == "LIST") Gallery::list(Serial);
    else if (upper == "SDINFO") Gallery::sdInfo(Serial);
    else if (upper == "CACHE STATS") { if (sdOK) FrameCache::printStats(Serial); }
    else if (upper == "CACHE CLEAR") { if (sdOK) FrameCache::clearAll(Serial); }
    else if (upper == "CACHE ON" || upper == "CACHE OFF") {
        settings.cacheEnabled = upper.endsWith("ON");
        Serial.printf("Cache: %s (RAM only)\n", settings.cacheEnabled ? "on" : "off");
    }
    else if (upper == "RESCAN") {
        sdOK = Gallery::begin();
        settings = Settings{};
        if (sdOK) Gallery::loadSettings(settings);
        Power::setBatteryCalibration(settings.batteryCalibration);
        status();
    } else if (upper == "NEXT") {
        if (displayNext()) scheduleWake();
    } else if (upper.startsWith("SHOW ")) {
        if (!sdOK || !Gallery::count()) return;
        const uint32_t idx = strtoul(line.c_str() + 5, nullptr, 10);
        const uint32_t previous = Gallery::nextIndex();
        Gallery::setNextIndex(idx);
        if (displayNext()) scheduleWake();
        else Gallery::setNextIndex(previous);
    } else if (upper == "TEST" || upper == "CLEAR") {
        digitalWrite(Board::LED_ACTIVITY, HIGH);
        if (sdOK) FrameCache::clearDisplayed();
        const bool ok = upper == "TEST" ? Epd::testPattern() : Epd::clear();
        digitalWrite(Board::LED_ACTIVITY, LOW);
        Serial.println(ok ? F("Display OK") : F("Display error"));
        if (ok) scheduleWake();
    } else if (upper == "LOWBAT") {
        if (sdOK) FrameCache::clearDisplayed();
        digitalWrite(Board::LED_ACTIVITY, HIGH);
        const bool ok = Epd::showLowBattery(settings.orientation);
        digitalWrite(Board::LED_ACTIVITY, LOW);
        Serial.println(ok ? F("Battery icon displayed") : F("Battery icon error"));
    } else if (upper == "TIME") {
        DateTime d;
        if (rtcOK && Rtc::now(d)) printDate(Serial, d);
        else Serial.println(F("RTC unavailable"));
    } else if (upper == "SETTIME") {
        Serial.println(F("Usage: SETTIME YYYY-MM-DD HH:MM:SS"));
        Serial.println(F("Example: SETTIME 2026-10-11 15:40:00"));
    } else if (upper.startsWith("SETTIME ")) {
        DateTime d;
        unsigned yr, mo, day, h, mi, se;
        if (sscanf(line.c_str() + 8, "%u-%u-%u %u:%u:%u", &yr, &mo, &day, &h, &mi, &se) == 6) {
            d.year = yr; d.month = mo; d.day = day; d.hour = h; d.minute = mi; d.second = se;
            if (rtcOK && Rtc::set(d)) { Serial.println(F("RTC updated")); scheduleWake(); }
            else Serial.println(F("Invalid date or RTC unavailable"));
        } else Serial.println(F("Usage: SETTIME 2026-10-11 12:30:00"));
    } else if (upper.startsWith("INTERVAL ")) {
        long n = strtol(line.c_str() + 9, nullptr, 10);
        if (n >= 1 && n <= 10080) { settings.refreshMinutes = n; scheduleWake(); }
        else Serial.println(F("Range: 1..10080"));
    } else if (upper == "LEDACT ON" || upper == "LEDACT OFF") {
        settings.activityLed = upper.endsWith("ON");
        digitalWrite(Board::LED_ACTIVITY, LOW);
        Serial.printf("LED ACT: %s (RAM only)\n", settings.activityLed ? "on" : "off");
    } else if (upper == "USB AUTO ON" || upper == "USB AUTO OFF") {
        settings.usbAutoNext = upper.endsWith("ON");
        Serial.printf("USB RTC auto: %s (RAM only; independent of physical switch)\n",
                      settings.usbAutoNext ? "on" : "off");
    } else if (upper.startsWith("ORIENTATION ")) {
        const String angle = upper.substring(12);
        if (angle == "0" || angle == "90" || angle == "180" || angle == "270") {
            settings.orientation = angle.toInt();
            Serial.printf("Orientation: %u degrees (RAM only)\n", settings.orientation);
        } else Serial.println(F("Valid: 0, 90, 180, 270"));
    } else if (upper == "SHUFFLE ON" || upper == "SHUFFLE OFF") {
        settings.shuffle = upper.endsWith("ON");
        Serial.printf("Shuffle: %s\n", settings.shuffle ? "on" : "off");
    } else if (upper == "SLEEP") {
        if (Power::usbPresent()) Serial.println(F("USB power present: cannot power off. Unplug USB."));
        else finishBatteryCycle();
    } else if (upper == "BOOTLOADER") {
        Serial.flush();
        reset_usb_boot(0, 0);
    } else Serial.println(F("Unknown command. Type HELP."));
}

void pollSerial() {
    while (Serial.available()) {
        const char c = Serial.read();
        if (c == '\r' || c == '\n') {
            runCommand(commandLine);
            commandLine = "";
        } else if (c >= 32 && c <= 126) {
            if (commandLine.length() < 120) commandLine += c;
            else commandLine = "";
        }
    }
}

void pollButton() {
    bool raw = digitalRead(Board::NEXT_BUTTON);
    if (raw != buttonLastRaw) {
        buttonLastRaw = raw;
        lastButtonChange = millis();
    }
    if (millis() - lastButtonChange > 40 && raw != buttonStable) {
        buttonStable = raw;
        if (!buttonStable) {
            Serial.println(F("[BUTTON] NEXT"));
            if (displayNext()) scheduleWake();
        }
    }
}
}

void setup() {
    Power::begin();
    Serial.begin(115200);
    delay(100);
    Serial.printf("\nPhotoPainter RP2040 v%s\n", FW_VERSION);
    randomSeed(analogRead(Board::BAT_ADC) ^ micros());
    // Hard cutoff before SD/EPD startup if battery is deeply discharged.
    if (!Power::usbPresent() && Power::batteryVoltage() < 3.1f) {
        Serial.println(F("[POWER] Emergency cutoff before SD/EPD init"));
        Power::cutPower();
        while (true) delay(1000);
    }
    rtcOK = Rtc::begin();
    if (rtcOK) Rtc::clearAlarm();
    // PWR is the charging indicator even before the first image is displayed.
    digitalWrite(Board::LED_POWER, Power::charging() ? HIGH : LOW);
    sdOK = Gallery::begin();
    if (sdOK) Gallery::loadSettings(settings);
    Power::setBatteryCalibration(settings.batteryCalibration);
    // ACT is now enabled on battery while processing images unless explicitly disabled.
    const float vbat = Power::batteryVoltage();
    const bool usb = Power::usbPresent();
    if (!usb && vbat < settings.lowBatteryV) {
        Serial.printf("[POWER] Low battery: %.2f V\n", vbat);
        if (rtcOK) Rtc::disableAlarm();
        if (vbat >= settings.criticalCutoffV) {
            // Five short flashes only when enough energy remains for the alert.
            Power::blink(Board::LED_POWER, 5, 90);
            if (sdOK) FrameCache::clearDisplayed();
            Serial.println(F("[POWER] Drawing empty battery warning"));
            if (!Epd::showLowBattery(settings.orientation)) Serial.println(F("[POWER] EPD warning failed"));
        } else {
            Serial.println(F("[POWER] Critical voltage: skipping EPD to avoid brownout"));
        }
        Power::cutPower();
        while (true) delay(1000);
    }
    buttonLastRaw = buttonStable = digitalRead(Board::NEXT_BUTTON);
    if (sdOK && Gallery::count() > 0) displayNext();
    else Serial.println(F("[SD] Missing SD or valid images; display unchanged"));
    if (!usb) finishBatteryCycle();
    else scheduleWake();
    status();
    help();
}

void loop() {
    pollSerial();
    pollButton();
    if (millis() - lastStatusPoll >= 1000) {
        lastStatusPoll = millis();
        digitalWrite(Board::LED_POWER, Power::charging() ? HIGH : LOW);
        // On battery, AUTO-NEXT gates the RTC wake signal in hardware.
        // On USB the MCU stays powered and cannot read the physical slider;
        // never bypass the hardware OFF position by polling RTC by default.
        // Users who want timed cycling on USB may opt in explicitly.
        if (rtcOK && settings.usbAutoNext && Power::usbPresent() && Rtc::alarmFired()) {
            Rtc::clearAlarm();
            displayNext();
            scheduleWake();
        }
        if (!Power::usbPresent()) finishBatteryCycle();
    }
    delay(10);
}
