# Credits and license notices / Crédits et mentions de licence

[English](#english) | [Français](#français) | [Shared original legal notices / Mentions légales originales communes](#shared-original-legal-notices)

## English

### Waveshare Electronics — original PhotoPainter firmware

- Official project: https://github.com/waveshareteam/PhotoPainter
- Original display driver: https://github.com/waveshareteam/PhotoPainter/blob/master/lib/e-Paper/EPD_7in3f.c
- Author credited in that driver: **Waveshare team** (2022-08-04, V1.0).

The GPIO layout and EPD register initialization sequences in this repository are adapted from the Waveshare driver. Its permission and disclaimer are retained in `src/Epd.cpp` and reproduced in the [shared original legal notices](#shared-original-legal-notices) below. This project is not an official Waveshare firmware.

### Third-party libraries

- **JPEGDEC** by Larry Bank (BitBank Software) — Apache-2.0: https://github.com/bitbank2/JPEGDEC
- **TJpg_Decoder** by Bodmer (based on ChaN's TJpgDec) — see upstream source: https://github.com/Bodmer/TJpg_Decoder
- **Arduino-Pico** by Earle F. Philhower III — see upstream license: https://github.com/earlephilhower/arduino-pico
- **PlatformIO** build system — https://platformio.org/

These dependencies are installed by PlatformIO; this archive does not vendor their implementation files.

### Project-specific license

See the unchanged [project-specific legal notice](#project-specific-legal-notice) below and `LICENSE` at the repository root.

## Français

### Waveshare Electronics — firmware PhotoPainter original

- Projet officiel : https://github.com/waveshareteam/PhotoPainter
- Pilote d'affichage original : https://github.com/waveshareteam/PhotoPainter/blob/master/lib/e-Paper/EPD_7in3f.c
- Auteur crédité dans ce pilote : **Waveshare team** (2022-08-04, V1.0).

La disposition des GPIO et les séquences d'initialisation des registres EPD de ce dépôt sont adaptées du pilote Waveshare. Son autorisation et sa clause de non-responsabilité sont conservées dans `src/Epd.cpp` et reproduites sans modification dans les [mentions légales originales communes](#shared-original-legal-notices) ci-dessous. Ce projet n'est pas un firmware officiel Waveshare.

### Bibliothèques tierces

- **JPEGDEC** par Larry Bank (BitBank Software) — Apache-2.0 : https://github.com/bitbank2/JPEGDEC
- **TJpg_Decoder** par Bodmer (basé sur TJpgDec de ChaN) — voir le code source amont : https://github.com/Bodmer/TJpg_Decoder
- **Arduino-Pico** par Earle F. Philhower III — voir la licence amont : https://github.com/earlephilhower/arduino-pico
- Système de compilation **PlatformIO** — https://platformio.org/

Ces dépendances sont installées par PlatformIO ; cette archive n'inclut pas leurs fichiers d'implémentation.

### Licence propre au projet

Voir la [mention légale propre au projet](#project-specific-legal-notice) ci-dessous, conservée sans modification, et `LICENSE` à la racine du dépôt. Les textes originaux anglais ci-dessous sont communs aux deux sections linguistiques et ne sont pas traduits afin de préserver exactement les mentions légales.

## Shared original legal notices

Mentions légales originales communes aux sections anglaise et française, reproduites à l'identique.

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

### Project-specific legal notice

Copyright 2026 PhotoPainter RP2040 contributors.

The original code authored for this PhotoPainter RP2040 repository is licensed under
Apache License, Version 2.0. See `LICENSE` at the repository root.

Third-party code and material (including portions adapted from Waveshare) keep their
respective notices and terms. The Apache-2.0 license for this project's original
code does not replace or override the upstream permission notices reproduced above.
