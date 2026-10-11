# PhotoPainter follow-up roadmap

Implemented in firmware/Studio 0.4.0:
- Shared native EPD export/reader with version, CRC32 and bounds validation
- Browser-based `settings.conf` editor and generation
- Web Serial diagnostic commands (STATUS, NEXT, SETTIME) in Chromium on HTTPS/localhost
- Source gallery sorted alphabetically, no-repeats stride-based pseudo-shuffle
- RTC quiet hours, battery ADC calibration coefficient
- Per-image processing settings, zoom/panning, PWA static caching
- GitHub Actions JS test and PlatformIO firmware build with UF2 artifact/release

**Requires hardware verification:** RP2040 compile with exact board, RTC timer across midnight, EPD transfer, power cutoff, physical AUTO-NEXT behavior, current consumption and actual ACeP palette rendition.

Proposed for future versions (not yet implemented):
- True Fisher–Yates persisted shuffle with configurable seed and full recovery after failure
- Color calibration from photographs of a known test chart under controlled illumination
- On-device microSD transfers and OTA-style firmware update over an authenticated framed serial protocol (Web Serial currently only issues commands)
- Configurable low-brightness hardware LED PWM subject to board validation
- Dynamic battery smoothing/hysteresis across boots and precise low-voltage benchmarking
- Optional JPEG enhanced decoder for full-quality progressive files
- Long-term endurance and regression testing on real hardware
