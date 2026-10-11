# Changelog

## 0.4.1 — GitHub Actions UF2 + Studio flash

- GitHub Actions unifié : tests, compilation PlatformIO RP2040, validation structurale UF2, publication Pages et release sur tag.
- Le build de `main` ajoute `PhotoPainter-RP2040-main.uf2` et les métadonnées vérifiables à GitHub Pages, sans fichier binaire versionné dans Git.
- Nouvelle page « Firmware & flash » dans PhotoPainter Studio : dernière release stable, dernier build de `main`, SHA-256 et guide BOOTSEL.
- Les releases `vX.Y.Z` vérifient que le tag correspond à `FW_VERSION` et contiennent UF2 + SHA-256.
- Tests de packaging UF2 et liens de firmware dans Studio.


## 0.4.0 — Native frames & frame settings

- Native `.epd` / `.edp` ready-to-display 4-bit ACeP-7 reader with header, checksum and palette validation before EPD power-up
- Native EPD export from Studio (landscape and portrait orientations) and full `pic/` ZIP packaging
- Studio frame settings editor: import/export `settings.conf`, quiet hours, threshold validation, Web Serial commands and RTC sync
- Studio optional per-photo processing, zoom/crop, PWA offline shell
- Firmware alphabetically sorted gallery, pseudo-random no-repeat step, RTC quiet interval, battery calibration
- JavaScript format and config tests, firmware build GitHub Action + UF2 artifact

Known constraints: full progressive JPEG decode still not provided; Web Serial cannot transfer images onto SD. Real board EPD, RTC, battery and PlatformIO compilation must be validated before claiming a stable hardware release.

## 0.3.4

- Baseline JPEG fallback via TJpgDec, serial diagnostics, persistent cache, battery/RTC functions
