# Changelog / Journal des modifications

[English](#english) | [Français](#français)

## English

### 0.4.1 — GitHub Actions UF2 + Studio flash

- Unified GitHub Actions: tests, PlatformIO RP2040 compilation, structural UF2 validation, Pages publishing and release on tag.
- The `main` build adds `PhotoPainter-RP2040-main.uf2` and verifiable metadata to GitHub Pages, without committing a binary file to Git.
- New “Firmware & flash” page in PhotoPainter Studio: latest stable release, latest `main` build, SHA-256 and BOOTSEL guide.
- `vX.Y.Z` releases verify that the tag matches `FW_VERSION` and include UF2 + SHA-256.
- UF2 packaging tests and firmware links in Studio.

### 0.4.0 — Native frames & frame settings

- Native `.epd` / `.edp` ready-to-display 4-bit ACeP-7 reader with header, checksum and palette validation before EPD power-up
- Native EPD export from Studio (landscape and portrait orientations) and full `pic/` ZIP packaging
- Studio frame settings editor: import/export `settings.conf`, quiet hours, threshold validation, Web Serial commands and RTC sync
- Studio optional per-photo processing, zoom/crop, PWA offline shell
- Firmware alphabetically sorted gallery, pseudo-random no-repeat step, RTC quiet interval, battery calibration
- JavaScript format and config tests, firmware build GitHub Action + UF2 artifact

Known constraints: full progressive JPEG decode still not provided; Web Serial cannot transfer images onto SD. Real board EPD, RTC, battery and PlatformIO compilation must be validated before claiming a stable hardware release.

### 0.3.4

- Baseline JPEG fallback via TJpgDec, serial diagnostics, persistent cache, battery/RTC functions

## Français

### 0.4.1 — GitHub Actions UF2 + flash Studio

- GitHub Actions unifié : tests, compilation PlatformIO RP2040, validation structurale UF2, publication Pages et release sur tag.
- Le build de `main` ajoute `PhotoPainter-RP2040-main.uf2` et les métadonnées vérifiables à GitHub Pages, sans fichier binaire versionné dans Git.
- Nouvelle page « Firmware & flash » dans PhotoPainter Studio : dernière release stable, dernier build de `main`, SHA-256 et guide BOOTSEL.
- Les releases `vX.Y.Z` vérifient que le tag correspond à `FW_VERSION` et contiennent UF2 + SHA-256.
- Tests de packaging UF2 et liens de firmware dans Studio.

### 0.4.0 — Images natives et paramètres du cadre

- Lecteur natif `.epd` / `.edp` d'images ACeP-7 4 bits prêtes à afficher, avec validation de l'en-tête, de la somme de contrôle et de la palette avant la mise sous tension de l'EPD
- Export EPD natif depuis Studio (orientations paysage et portrait) et création d'une archive ZIP complète de `pic/`
- Éditeur des paramètres du cadre dans Studio : import/export de `settings.conf`, heures de silence, validation des seuils, commandes Web Serial et synchronisation RTC
- Traitement facultatif par photo dans Studio, zoom/recadrage, interface PWA disponible hors ligne
- Galerie du firmware triée alphabétiquement, sélection pseudo-aléatoire sans répétition, intervalle de silence RTC, étalonnage de la batterie
- Tests JavaScript du format et de la configuration, GitHub Action de compilation du firmware + artefact UF2

Contraintes connues : le décodage complet des JPEG progressifs n'est toujours pas fourni ; Web Serial ne peut pas transférer d'images sur la carte SD. L'EPD, le RTC et la batterie sur une carte réelle, ainsi que la compilation PlatformIO, doivent être validés avant de déclarer une version matérielle stable.

### 0.3.4

- Décodage de secours des JPEG baseline via TJpgDec, diagnostics série, cache persistant, fonctions batterie/RTC
