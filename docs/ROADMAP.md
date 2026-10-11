# PhotoPainter follow-up roadmap

[English](#english) | [Français](#francais) · [README](../README.md) · [Firmware](FIRMWARE.md) · [UF2](FLASH_UF2.md) · [Roadmap / Feuille de route](ROADMAP.md) · [Tests](TEST_PLAN.md)

<a id="english"></a>

## English

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


[Français ↓](#francais) · [Top ↑](#english)

---

<a id="francais"></a>

## Français

# Feuille de route PhotoPainter

Implémenté dans le firmware/Studio 0.4.0 :
- Export/lecture EPD natif partagés avec version, CRC32 et validation des limites
- Édition et génération de `settings.conf` dans le navigateur
- Commandes de diagnostic Web Serial (STATUS, NEXT, SETTIME) dans Chromium sur HTTPS/localhost
- Galerie source triée alphabétiquement, pseudo-mélange par pas sans répétition
- Heures de silence RTC, coefficient de calibration ADC batterie
- Réglages par image, zoom/déplacement, cache statique PWA
- Tests JS GitHub Actions et compilation PlatformIO avec artefact/release UF2

**Validation matérielle nécessaire :** compilation RP2040 pour la carte exacte, minuterie RTC à travers minuit, transfert EPD, coupure d’alimentation, comportement physique AUTO-NEXT, consommation et rendu réel de la palette ACeP.

Propositions pour les versions futures (non implémentées) :
- Véritable mélange Fisher–Yates persistant avec graine configurable et récupération complète après échec
- Calibration des couleurs à partir de photographies d’une mire connue sous éclairage contrôlé
- Transferts vers la microSD embarquée et mise à jour du firmware de type OTA sur protocole série authentifié et encadré (Web Serial ne transmet actuellement que des commandes)
- PWM matériel configurable à faible luminosité pour les LED, sous réserve de validation de la carte
- Lissage/hystérésis dynamique de la batterie entre démarrages et mesures précises à basse tension
- Décodeur JPEG amélioré optionnel pour les fichiers progressifs en pleine qualité
- Tests d’endurance et de non-régression à long terme sur matériel réel

[English ↑](#english) · [Français ↑](#francais)
