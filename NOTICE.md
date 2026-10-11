# Credits and license notices

## Waveshare Electronics — original PhotoPainter firmware

- Official project: https://github.com/waveshareteam/PhotoPainter
- Original display driver: https://github.com/waveshareteam/PhotoPainter/blob/master/lib/e-Paper/EPD_7in3f.c
- Author credited in that driver: **Waveshare team** (2022-08-04, V1.0).

The GPIO layout and EPD register initialization sequences in this repository are adapted from the Waveshare driver. Its permission and disclaimer are retained in `src/Epd.cpp` and reproduced below. This project is not an official Waveshare firmware.

### Original driver permission notice

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documnetation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

## Third-party libraries

- **JPEGDEC** by Larry Bank (BitBank Software) — Apache-2.0: https://github.com/bitbank2/JPEGDEC
- **TJpg_Decoder** by Bodmer (based on ChaN's TJpgDec) — see upstream source: https://github.com/Bodmer/TJpg_Decoder
- **Arduino-Pico** by Earle F. Philhower III — see upstream license: https://github.com/earlephilhower/arduino-pico
- **PlatformIO** build system — https://platformio.org/

These dependencies are installed by PlatformIO; this archive does not vendor their implementation files.

## Project-specific license

Copyright 2026 PhotoPainter RP2040 contributors.

The original code authored for this PhotoPainter RP2040 repository is licensed under
Apache License, Version 2.0. See `LICENSE` at the repository root.

Third-party code and material (including portions adapted from Waveshare) keep their
respective notices and terms. The Apache-2.0 license for this project's original
code does not replace or override the upstream permission notices reproduced above.

