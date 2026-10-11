# PhotoPainter RP2040 firmware — technical documentation (v0.4.1)

[English](#english) | [Français](#francais) · [README](../README.md) · [Firmware](FIRMWARE.md) · [UF2](FLASH_UF2.md) · [Roadmap / Feuille de route](ROADMAP.md) · [Tests](TEST_PLAN.md)

<a id="english"></a>

## English

Original repository: [Waveshare PhotoPainter](https://github.com/waveshareteam/PhotoPainter). This firmware is an unofficial community adaptation. Back: [main README](../README.md).

> **Target hardware:** PhotoPainter (F) RP2040, seven-color ACeP, 800 × 480. No compatibility is claimed with RP2350 or ESP32-S3 models.

## Features

- `.jpg`, `.jpeg` (baseline JPEG, plus a compatibility preview for progressive files) and `.bmp` photos read **directly from microSD**; native `.epd`/`.edp` frames are also supported.
- **JPG/JPEG and 24-bit BMP resized** to 800×480, or 480×800 for portrait orientation. `image_fit=contain` (white borders) or `cover` (central crop).
- **SD /cache/** in display format: 4 bits per pixel (192000 bytes + 40-byte header), CRC32 and source-file size/FAT date/sample checks.
- On a cache HIT, direct transfer of 192000 bytes to the panel, **without JPEGDEC, resizing or color quantization**.
- E Ink refresh skipped when the selected photo is **already displayed**; forced processing when the image, orientation or crop changes.
- With one photo, optional RTC alarm disabling (`single_image_auto_pause=true`). No automatic wakeups; wake the frame manually to discover new photos.
- **ACT** LED active during image reading, conversion and refresh, including on battery (`activity_led=true` by default). Disable it to save power. **PWR** LED on while charging, off when charging finishes, and five flashes for noncritical low battery.
- Scans `/pic`, then `/PIC`, `/Pic`, and, if none contains image files, the **card root**. `SDINFO` lists directories to diagnose `images: 0`.
- PlatformIO monitor: visible typing (`monitor_echo=yes`) and submission with Enter (`monitor_filters=send_on_enter`). Diagnostics print voltages correctly without `%f`, which some builds do not support.
- `settings.conf` at the **SD root**, with orientation `0/90/180/270` and refresh interval.
- Low battery: **large empty battery crossed out in red across the screen**, followed by hardware shutdown and RTC alarm disabling. PWR flashes five times if voltage still permits.
- Critical battery protection: below `critical_cutoff_v`, no refresh, reducing voltage-collapse or reboot-loop risk.
- USB commands `LOWBAT` (icon preview), `ORIENTATION` and expanded `STATUS`.

## Installation

1. VS Code + PlatformIO: open the project folder.
2. `Build`, then `Upload`, serial baud rate **115200**. Arduino-Pico platform; PlatformIO downloads `JPEGDEC 1.8.4` and `TJpg_Decoder` automatically.
3. If automatic upload fails: hold **BOOT**, press and release **RUN**, release BOOT, then copy the `.uf2` to `RPI-RP2`.
4. Format a **FAT32 microSD** and copy the contents of `sdcard/` to its root. Ordinary JPEG files can go directly in `/pic/`.
5. With USB connected, open the **115200** serial monitor, type `HELP`, then Enter. Run `SDINFO`, `RESCAN`, `LIST`, `NEXT` if `images: 0`.

Layout:

```text
SD:/
├── settings.conf       # Persistent configuration
├── pic/
│   ├── 00000_demo.bmp
│   ├── 00001_demo.jpg
│   └── vacances.jpeg
├── cache/
│   ├── 87654321.epc    # Photo cache (name derived from path)
│   └── ...
├── last.epd            # Signature of last confirmed display
├── index.txt           # Next photo position
└── render.tmp          # Optional temporary file, cleaned after processing
```

The firmware never modifies original photos. `render.tmp` needs approximately **192 kB free** if the 192 kB framebuffer cannot fit in RAM. Do not remove the SD during display processing.

## `settings.conf` configuration

```ini
orientation=0
refresh_minutes=720
shuffle=false
image_fit=contain
progressive_jpeg=preview
cache_enabled=true
skip_unchanged=true
single_image_auto_pause=true
activity_led=true
usb_auto_next=false
cache_write_min_v=3.6
low_battery_v=3.4
critical_cutoff_v=3.1
quiet_hours_start=00:00
quiet_hours_end=00:00
battery_calibration=1.000
```

| Key | Values | Effect |
|---|---|---|
| `orientation` | `0`, `90`, `180`, `270` | Clockwise rotation; 90/270 => portrait logical canvas 480×800. Native EPD frames already use physical panel order and are not rotated again. |
| `refresh_minutes` | `1` to `10080` | Minutes between images; requires a set RTC and enabled AUTO-NEXT. |
| `shuffle` | `true` / `false` | Pseudo-shuffles image order without immediate repeats when multiple images exist. |
| `progressive_jpeg` | `preview` / `reject` | `preview`: enlarged 1/8 DC thumbnail of a progressive JPEG (low quality), then cached. `reject`: refuses progressive JPEGs. For full quality, convert to baseline JPEG on a computer. |
| `image_fit` | `contain` / `cover` | JPEG **and BMP**: full image with borders / filled screen with cropping. `jpeg_fit` is an accepted alias. |
| `cache_enabled` | `true` / `false` | Saves conversion, then reads the cache on subsequent displays. |
| `skip_unchanged` | `true` / `false` | Does not refresh E Ink when the same image and settings are already displayed. |
| `single_image_auto_pause` | `true` / `false` | With one image, disables RTC wakeups until manual wake. Disable to check SD changes regularly. |
| `activity_led` | `true` / `false` | Lights ACT during SD reading, conversion and refresh, including on battery. `false` reduces consumption. |
| `usb_auto_next` | `true` / `false` | On USB, explicitly enables the software RTC slideshow **independently** of the hardware switch. Default `false`: manual NEXT only. |
| `cache_write_min_v` | 3.3 to 4.2 V | Writes cache only above this battery threshold (or with USB present); never below `low_battery_v + 0.1 V`. |
| `low_battery_v` | 3.2 to 3.8 V | Below this, attempts the empty-battery screen and cuts power. |
| `critical_cutoff_v` | 3.0 to 3.4 V | Below this, shuts down without refresh; must remain below the preceding threshold. |
| `quiet_hours_start`, `quiet_hours_end` | `HH:MM` | Equal values disable quiet hours; intervals crossing midnight are supported. Scheduled wakeups inside the interval are deferred to its end. |
| `battery_calibration` | `0.85` to `1.15` | ADC voltage multiplier, default `1.000`; not a battery state-of-charge gauge. |

`settings.conf` replaces `frame.ini`, but `frame.ini` and its old `interval_minutes` key remain a **fallback** for existing installations. Unknown keys are ignored. `INTERVAL`, `SHUFFLE`, `ORIENTATION` change RAM settings only until the next boot.

**Interval and switch:** a complete panel refresh takes several tens of seconds. The alarm is scheduled `refresh_minutes` after processing, adjusted for quiet hours. On battery, **AUTO-NEXT is a hardware switch gating the RTC signal**: **ON** lets the alarm power up the board; **OFF** prevents alarm wakeups. Firmware cannot directly read the switch position (no documented selection GPIO). **NEXT** works in both positions. On **USB**, the RP2040 stays powered and can detect alarms in software without this switch; `usb_auto_next=false` disables this bypass, while `usb_auto_next=true` enables periodic cycling even with the switch OFF. Test ON/OFF **on battery**. With `single_image_auto_pause=true` and one photo, the alarm is deliberately disabled even with AUTO-NEXT ON.

## Image formats

- **JPG/JPEG:** baseline color or grayscale JPEG compatible with JPEGDEC; decoded from SD and converted to the ACeP palette (black, white, blue, green, red, orange, yellow). A separate TJpg baseline fallback is available. **Progressive** JPEG: `progressive_jpeg=preview` permits a **1/8-resolution preview** (first DC scan only), enlarged on screen and cached. This is **not** full-resolution progressive decoding; quality can be substantially degraded. Some unusual JPEGs remain incompatible. For quality photos, use the computer tool to produce baseline JPEG at 800 × 480.
- **BMP:** uncompressed **24-bit RGB**, arbitrary reasonable dimensions (maximum 16384 pixels per axis), resampled on-board with `contain`/`cover`. Indexed, RLE and 32-bit formats are unsupported.
- **Native EPD:** `.epd` or `.edp`, PPEF v1 header and packed physical-panel pixels; see [EPD format](EPD_FORMAT.md). Orientation/cropping must be prepared before export.
- Firmware has **no automatic EXIF orientation handling**. `orientation` rotates content for the frame installation. PhotoPainter Studio Web (`tools/`) can normalize phone-photo EXIF rotation when the browser applies it.
- Seven-color display; color reproduction is necessarily more limited than the original JPEG. No partial updates (color ACeP).
- The gallery sorts alphabetically up to 512 images. Failed decoding preserves the displayed photo; with multiple images, the next attempt skips the failed file instead of getting stuck.

### Recommended photo preparation

Firmware can decode compatible baseline JPEGs, but preconverted images give better results and load faster. **[PhotoPainter Studio Web](../tools/)** prepares photos in a browser, corrects EXIF orientation when applied by the browser, and offers eleven dithering algorithms.

To export to microSD: open PhotoPainter Studio, import images, adjust rendering, click **Export all as ZIP**, then extract `pic/` to the SD. Configuration remains in `settings.conf` at the SD root.

Progressive JPEGs are not decoded at full resolution by firmware: **PhotoPainter Studio native EPD (or BMP) export is preferable**. Built-in progressive `preview` uses a reduced preview.

## Cache and battery life (v0.3)

Each source file remains in `/pic/`. For converted BMP/JPEG, the RP2040 saves only an image **already resized, rotated and converted to seven colors**, raw **800×480 × 4 bits = 192000 bytes**, plus 40 metadata bytes. Written to `/cache/xxxxxxxx.epc` through a temporary file renamed after writing.

On wake:

1. Checks voltage before heavy operations. SD mounts only if voltage is not critical.
2. Loads `settings.conf`, lists `/pic` (with directory fallbacks), identifies the next photo by path, size, FAT date and sampled start/end bytes.
3. If this image is still displayed (`last.epd`) and `skip_unchanged=true`, **does not wake the panel**.
4. Otherwise, if the cache is valid, transfers 192000 bytes directly over SPI, checking CRC32 before triggering refresh.
5. Otherwise, resizes BMP/JPEG; large JPEGs use reduced JPEGDEC decoding (1/2, 1/4, 1/8), then builds cache **once** when battery voltage permits.
6. The screen remains visible without power. Sleeps the E Ink controller and cuts hardware power via GPIO18 until the next RTC wake (unless paused for a single image).

Cache becomes stale if the image, orientation, `image_fit` or firmware format changes. FAT32 has coarse modification-time precision; start/end sampling reduces false cache HITs without reading the entire JPEG. If in doubt, `CACHE CLEAR` forces reconversion.

**Maximum savings:** with one photo and `single_image_auto_pause=true`, RTC alarm is disabled. Adding photos without restarting the frame will not discover them automatically. For a changing album, set this to `false`; `skip_unchanged` still prevents unnecessary refreshes.

Cache does not accelerate the physical ACeP pigment refresh: a new image still requires this slow, energy-intensive step. Savings mainly come from **avoided decoding** and especially **eliminated unnecessary wakeups/refreshes**. No battery-life figure is guaranteed without measurements on the real board.

## Battery and safety

Battery is measured through ADC29 (Waveshare ×3 divider). Default thresholds:

1. **Above 3.4 V:** normal slideshow; ACT on during processing if `activity_led=true`.
2. **From 3.1 to 3.4 V**, battery-only: five short PWR flashes, attempted **crossed-out empty battery** display, alarm disabling and GPIO18 cutoff command.
3. **Below 3.1 V:** shutdown without refresh, avoiding further discharge.

Important: if the battery is **already exhausted**, physically guaranteeing a panel refresh is impossible. The warning must happen **before** critical voltage to show the icon. Test and adjust thresholds for the actual battery; **do not deliberately discharge the cell** for testing. Connect USB and send `LOWBAT` to preview safely.

The USB cycle does not cut power. On battery, the frame wakes the RP2040 and cuts power after displaying. The CR1220 coin cell must be functional to preserve RTC time.

## RTC and USB commands

Initially set local time over USB:

```text
SETTIME 2026-10-11 12:30:00
TIME
STATUS
```

Firmware does not reset the clock to a fixed date at every boot. Without built-in networking, there is no NTP or automatic daylight-saving adjustment.

| Command | Effect |
|---|---|
| `HELP`, `STATUS`, `LIST`, `RESCAN`, `SDINFO` | Help/diagnostics; `SDINFO` lists root and `/pic`; `RESCAN` recounts images |
| `CACHE STATS`, `CACHE CLEAR`, `CACHE ON`, `CACHE OFF` | Statistics, cache purge, volatile enable/disable |
| `NEXT`, `SHOW 0` | Displays next image or an indexed image |
| `TEST`, `CLEAR`, `LOWBAT` | Color chart, white screen, battery icon |
| `TIME`, `SETTIME YYYY-MM-DD HH:MM:SS` | RTC clock |
| `ORIENTATION 90`, `INTERVAL 60`, `SHUFFLE ON` | Volatile changes until reboot |
| `LEDACT ON`/`OFF`, `USB AUTO ON`/`OFF` | Temporary ACT and USB RTC-trigger settings |
| `SLEEP`, `BOOTLOADER` | Battery shutdown or UF2 flash mode |

## Pinout

| Module | GPIO |
|---|---|
| EPD (DC, CS, SCK, MOSI, RST, BUSY, POWER) | 8, 9, 10, 11, 12, 13, 16 |
| SD (SCK, MOSI, MISO, CS) | 2, 3, 4, 5 |
| PCF85063 RTC (SDA, SCL, INT) | 14, 15, 6 |
| Charge, BAT cutoff, NEXT button, VBUS, battery ADC | 17, 18, 19, 24, 29 |
| ACT LED, PWR LED | 25, 26 |

## References

- Original Waveshare PhotoPainter RP2040: https://github.com/waveshareteam/PhotoPainter
- JPEGDEC decoder: https://github.com/bitbank2/JPEGDEC
- Arduino-Pico core: https://arduino-pico.readthedocs.io/en/latest/platformio.html

See [hardware test plan](TEST_PLAN.md). **This documentation update does not constitute a PlatformIO build or real-board validation.**

## Missing-image / USB-console diagnostics (v0.3.3)

`SD: ok | images: 0` means SD mounted **but no usable image filename was found**: JPEG decoding has not started. Send `SDINFO` (root and `/pic` listing), then `RESCAN`, `LIST`, `NEXT`. Accepted extensions are `.jpg`, `.jpeg`, `.bmp`, `.epd`, `.edp`, case-insensitive. Check actual FAT32 (not exFAT), directory names, and avoid subfolders in `/pic` (nonrecursive scan). Scans `/pic`, then `/PIC`, `/Pic`, then root only if no images were previously found.

In `platformio.ini`:

```ini
monitor_speed = 115200
monitor_echo = yes
monitor_eol = LF
monitor_filters = send_on_enter
```

PlatformIO monitor has no input box: click the **monitor terminal**, type `SDINFO`, then Enter. Commands are sent only after Enter. If RTC reports `RTC: 2053` or another wrong date, use `SETTIME 2026-10-11 14:30:00` (adapt to local time), then `TIME`.

**Hardware limitation:** full-resolution progressive JPEG needs substantial coefficient memory. On RP2040 (264 kB RAM), JPEGDEC 1.8.4 supports only the first DC scan. `preview` favors compatibility; for sharp images, prepare with PhotoPainter Studio Web (`tools/`) once, since `/cache` eliminates processing on subsequent displays.

## v0.3.3 — JPEG from SD

- Corrects JPEGDEC callbacks: `JPEGFILE.iPos` must track reads and seeks. Otherwise some images fail and later reads no longer match the real stream.
- Explicitly seeks back to offset 0 after computing the cache signature.
- Checks JPEG header `FF D8` and prints JPEGDEC error code and first 12 bytes if parsing fails. A `.jpg` containing WebP, AVIF or HTML is reported.
- Undecodable files no longer trap NEXT indefinitely: the next attempt selects another gallery item. Failure does not erase the displayed photo.

**Debugging:** `RESCAN`, `LIST`, `NEXT`, then copy `[JPEG]` lines (including error code/signature) for missing images. PlatformIO `No new RPxxxx device found yet` does not prove a flash failure if followed by `Verifying Flash: OK` and application restart.

## v0.4.0 — native files and quiet hours

- `.epd` or `.edp`: 24-byte PPEF v1 header followed by 192000 bytes of ACeP-7 4-bit data in physical panel order. No intermediate framebuffer or RP2040 quantization. See [EPD_FORMAT.md](EPD_FORMAT.md).
- `settings.conf` adds `quiet_hours_start=HH:MM`, `quiet_hours_end=HH:MM` (equal = disabled; midnight crossing supported) and `battery_calibration=1.000` (0.85..1.15).
- Gallery sorts alphabetically up to 512 images. Shuffle uses a pseudo-random stride coprime with image count to ensure a nonrepeating cycle. This is not uniform Fisher–Yates.
- AUTO-NEXT remains **a hardware RTC wake mechanism**; software cannot read its state.
- `STATUS` prints quiet hours and ADC calibration coefficient. Measurement is not a battery gauge with a runtime curve.


[Français ↓](#francais) · [Top ↑](#english)

---

<a id="francais"></a>

## Français

# Firmware PhotoPainter RP2040 — documentation technique (v0.4.1)

Dépôt d'origine : [Waveshare PhotoPainter](https://github.com/waveshareteam/PhotoPainter). Ce firmware est une adaptation communautaire non officielle. Retour : [README principal](../README.md).

> **Matériel cible** : PhotoPainter (F) RP2040, ACeP 7 couleurs, 800 × 480. Pas de compatibilité annoncée avec les modèles RP2350 et ESP32-S3.

## Nouveautés

- Photos `.jpg`, `.jpeg` (JPEG baseline, plus aperçu de compatibilité pour les progressifs) et `.bmp` en lecture **directe sur la microSD**.
- **JPG/JPEG et BMP 24 bits redimensionnés** vers 800×480, ou 480×800 pour l’orientation portrait. `image_fit=contain` (bordures blanches) ou `cover` (recadrage central).
- **Cache SD /cache/** au format écran : 4 bits par pixel (192 000 octets + 40 octets d’en-tête), CRC32 et vérification taille/date FAT/échantillon du fichier source.
- En cache HIT, transmission directe de 192 000 octets à la dalle, **sans JPEGDEC, redimensionnement ou quantification des couleurs**.
- Rafraîchissement E Ink évité si la photo sélectionnée est **déjà celle affichée** ; reprise forcée si l’image, l’orientation ou le recadrage changent.
- Avec une seule photo, option de désactivation de l’alarme RTC (`single_image_auto_pause=true`). Plus aucun réveil automatique ; le cadre devra être réactivé manuellement pour voir de nouvelles photos.
- LED **ACT** active pendant la lecture, la conversion et le rafraîchissement des images, y compris sur batterie (`activity_led=true` par défaut). Option pour l’éteindre et gagner en autonomie. LED **PWR** allumée pendant la charge, éteinte en fin de charge et 5 flashes en cas de batterie basse non critique.
- Le scan regarde `/pic`, puis `/PIC`, `/Pic` et, si aucun fichier image n’y est trouvé, directement la **racine de la carte**. `SDINFO` affiche le contenu des répertoires pour diagnostiquer `images: 0`.
- Moniteur PlatformIO : saisie visible (`monitor_echo=yes`) et validation par Entrée (`monitor_filters=send_on_enter`). Le diagnostic affiche correctement les tensions (sans le format `%f` non pris en charge dans certaines builds).
- `settings.conf` à la **racine de la SD** avec orientation `0/90/180/270` et intervalle de rafraîchissement.
- Batterie basse : **grosse batterie vide barrée de rouge sur tout l'écran**, puis arrêt matériel et désactivation de l'alarme RTC. La LED PWR clignote 5 fois (si la tension est encore suffisante).
- Protection batterie critique : sous `critical_cutoff_v`, pas de rafraîchissement pour limiter le risque de chute de tension ou de redémarrage en boucle.
- Commandes USB `LOWBAT` (prévisualiser l'icône), `ORIENTATION` et `STATUS` enrichi.

## Installation

1. VS Code + PlatformIO : ouvrir le dossier du projet.
2. `Build`, puis `Upload`, port série **115200**. Plateforme Arduino-Pico ; `JPEGDEC 1.8.4` et `TJpg_Decoder` sont téléchargés automatiquement par PlatformIO.
3. Si l'upload automatique échoue : maintenir **BOOT**, presser et relâcher **RUN**, relâcher BOOT, puis déposer le `.uf2` sur `RPI-RP2`.
4. Formater une **microSD FAT32** et copier le contenu du dossier `sdcard/` à sa racine. Des fichiers JPEG normaux peuvent être copiés directement dans `/pic/`.
5. Avec USB branché, ouvrir le moniteur série **115200**, taper `HELP`, puis Entrée. Lancer `SDINFO`, `RESCAN`, `LIST`, `NEXT` en cas de `images: 0`.

Structure :

```text
SD:/
├── settings.conf       # Configuration persistante
├── pic/
│   ├── 00000_demo.bmp
│   ├── 00001_demo.jpg
│   └── vacances.jpeg
├── cache/
│   ├── 87654321.epc    # Cache de photo (nom dérivé du chemin)
│   └── ...
├── last.epd            # Signature du dernier affichage validé
├── index.txt           # Position de la prochaine photo
└── render.tmp          # Temporaire éventuel, nettoyé après traitement
```

Le firmware ne modifie jamais les photos d'origine. `render.tmp` nécessite environ **192 Ko libres** si le framebuffer de 192 Ko ne rentre pas en RAM. Ne pas retirer la SD pendant l'affichage.

## Configuration `settings.conf`

```ini
orientation=0
refresh_minutes=720
shuffle=false
image_fit=contain
progressive_jpeg=preview
cache_enabled=true
skip_unchanged=true
single_image_auto_pause=true
activity_led=true
usb_auto_next=false
cache_write_min_v=3.6
low_battery_v=3.4
critical_cutoff_v=3.1
quiet_hours_start=00:00
quiet_hours_end=00:00
battery_calibration=1.000
```

| Clé | Valeurs | Effet |
|---|---|---|
| `orientation` | `0`, `90`, `180`, `270` | Rotation dans le sens horaire ; 90/270 => toile logique portrait 480×800. Les EPD natifs sont déjà dans l’ordre physique et ne sont pas tournés à nouveau. |
| `refresh_minutes` | `1` à `10080` | Minutes entre deux images ; nécessite RTC réglé et AUTO-NEXT activé. |
| `shuffle` | `true` / `false` | Mélange l'ordre des images sans répétition immédiate si plusieurs images. |
| `progressive_jpeg` | `preview` / `reject` | `preview` : JPEG progressif affiché depuis une miniature DC 1/8 agrandie (qualité faible), puis mis en cache. `reject` : refuse les JPEG progressifs. Pour la pleine qualité : conversion sur PC en JPEG baseline. |
| `image_fit` | `contain` / `cover` | Pour JPEG **et BMP** : image entière avec bordures / écran rempli avec recadrage. `jpeg_fit` est un alias accepté. |
| `cache_enabled` | `true` / `false` | Persiste la conversion, puis lit le cache les fois suivantes. |
| `skip_unchanged` | `true` / `false` | Ne rafraîchit pas l’E Ink si la même image et les mêmes réglages sont déjà affichés. |
| `single_image_auto_pause` | `true` / `false` | Si une seule image existe, coupe les réveils RTC jusqu’à un réveil manuel. Désactiver pour vérifier régulièrement les changements SD. |
| `activity_led` | `true` / `false` | Allume ACT pendant la lecture SD, la conversion et le rafraîchissement écran, y compris sur batterie. `false` pour réduire la consommation. |
| `usb_auto_next` | `true` / `false` | Sur USB, active explicitement le diaporama RTC par logiciel, **indépendamment** du switch matériel. `false` par défaut : NEXT manuel seulement. |
| `cache_write_min_v` | 3,3 à 4,2 V | N’écrit le cache que si la batterie dépasse ce seuil (ou USB présent) ; jamais inférieur à `low_battery_v + 0,1 V`. |
| `low_battery_v` | 3,2 à 3,8 V | En dessous, tente l'écran de batterie vide et coupe l'alimentation. |
| `critical_cutoff_v` | 3,0 à 3,4 V | En dessous, coupe sans rafraîchir ; doit rester inférieur au seuil précédent. |
| `quiet_hours_start`, `quiet_hours_end` | `HH:MM` | Valeurs identiques : désactivé ; passage à minuit supporté. Les réveils prévus dans cette période sont reportés à sa fin. |
| `battery_calibration` | `0.85` à `1.15` | Multiplicateur ADC, défaut `1.000` ; pas une jauge de charge. |

`settings.conf` remplace `frame.ini`, mais `frame.ini` et son ancienne clé `interval_minutes` restent lus **en secours** pour les installations existantes. Les clés non reconnues sont ignorées. Les commandes `INTERVAL`, `SHUFFLE`, `ORIENTATION` changent seulement les paramètres en RAM jusqu'au prochain boot.

**Intervalle et switch :** un rafraîchissement complet de cette dalle prend plusieurs dizaines de secondes. L'alarme est programmée `refresh_minutes` après le traitement, avec ajustement pour les heures de silence. Sur batterie, le **switch AUTO-NEXT est un interrupteur matériel du signal RTC** : sur **ON**, l'alarme peut remettre la carte sous tension ; sur **OFF**, l'alarme ne réveille pas la carte. Le firmware ne peut pas lire directement la position du switch (aucun GPIO de sélection documenté). **NEXT** continue de fonctionner dans les deux positions. Sur **USB**, le RP2040 reste alimenté et le signal d'alarme peut être détecté par logiciel sans passer par ce switch ; `usb_auto_next=false` désactive ce contournement, et `usb_auto_next=true` active le cycle périodique même si le switch est sur OFF. Il est donc recommandé de tester le mode ON/OFF **sur batterie**. Avec `single_image_auto_pause=true` et une seule photo, l'alarme est volontairement désactivée, même si AUTO-NEXT est sur ON.

## Formats d'image

- **JPG/JPEG** : JPEG baseline couleur ou niveaux de gris compatibles avec JPEGDEC ; un décodeur baseline TJpg de secours est également disponible ; le décodeur lit depuis la SD et convertit les couleurs vers la palette ACeP (noir, blanc, bleu, vert, rouge, orange, jaune). JPEG **progressif** : `progressive_jpeg=preview` autorise une **prévisualisation 1/8 de résolution** (première passe DC seulement), agrandie sur l’écran puis conservée en cache. Ce n’est **pas** un décodage progressif pleine définition et la qualité peut être nettement dégradée. Certains JPEG atypiques resteront non compatibles. Pour une photo de qualité : utiliser l’outil PC pour obtenir un JPEG baseline en 800 × 480.
- **BMP** : RGB **24 bits non compressé**, dimensions quelconques raisonnables (au maximum 16384 pixels par axe) : rééchantillonnage sur la carte en `contain`/`cover`. Les formats indexés, RLE et 32 bits ne sont pas supportés.
- **EPD natif** : `.epd` ou `.edp`, en-tête PPEF v1 et pixels dans l’ordre physique ; voir [format EPD](EPD_FORMAT.md). Rotation et recadrage sont préparés avant export.
- Il n'y a **pas de gestion automatique de l'orientation EXIF** dans le firmware. `orientation` effectue la rotation du contenu pour l'installation du cadre. Pour la rotation EXIF des photos de téléphone, PhotoPainter Studio Web (dans `tools/`) peut normaliser l'image.
- Affichage 7 couleurs ; le résultat colorimétrique est forcément plus limité que la photo JPEG d'origine. Pas de mise à jour partielle (ACeP couleur).
- La galerie trie alphabétiquement jusqu’à 512 images. En cas d’échec, l’image affichée reste en place ; avec plusieurs images, la prochaine tentative saute le fichier erroné.

### Préparation recommandée des photos

Le firmware peut décoder certains JPEG baseline, mais les images préconverties donnent de meilleurs résultats et chargent plus vite. Le convertisseur **[PhotoPainter Studio Web](../tools/)** prépare les photos depuis un navigateur, corrige l'orientation EXIF lorsque le navigateur l'applique et propose onze algorithmes de dithering.

Pour exporter vers la microSD : ouvrir PhotoPainter Studio, importer les images, ajuster le rendu, cliquer sur **Exporter tout en ZIP**, puis extraire le dossier `pic/` sur la carte SD. La configuration reste dans `settings.conf` à la racine de la SD.

Les JPEG progressifs ne sont pas décodés en pleine résolution par le firmware : **l'export EPD natif (ou BMP) de PhotoPainter Studio est préférable**. Le mode `preview` des JPEG progressifs intégrés au firmware utilise un aperçu réduit.

## Cache et autonomie (v0.3)

Chaque fichier source est conservé dans `/pic/`. Pour les BMP/JPEG convertis, le RP2040 n’enregistre qu’une image **déjà redimensionnée, tournée et convertie à 7 couleurs**, au format brut **800×480 × 4 bits = 192 000 octets**, avec 40 octets de métadonnées. Enregistrement dans `/cache/xxxxxxxx.epc`, par création d’un fichier temporaire puis renommage en fin d’écriture.

Au réveil :

1. Vérifie la tension avant toute utilisation lourde. La carte SD est montée seulement si la tension n’est pas critique.
2. Charge `settings.conf`, liste `/pic` (avec dossiers de secours), identifie la prochaine photo par chemin, taille, date FAT et quelques octets de début/fin.
3. Si cette image est encore affichée (`last.epd`) et `skip_unchanged=true`, **ne réveille pas la dalle**.
4. Sinon, si le cache est valide, transfère directement les 192 000 octets en SPI, en vérifiant un CRC32 avant de déclencher le rafraîchissement.
5. Sinon, redimensionne BMP/JPEG ; pour les grands JPEG, utilise le décodage JPEGDEC réduit (1/2, 1/4, 1/8), puis construit le cache **une seule fois** quand la batterie est suffisamment chargée.
6. L’écran reste visible sans alimentation. Mise en veille du contrôleur E Ink et coupure matérielle via GPIO18 jusqu’au prochain réveil RTC (sauf pause sur image unique).

Le cache est automatiquement considéré périmé si l’image change, si l’orientation ou `image_fit` change, ou si le format du firmware évolue. FAT32 n’a qu’une faible précision sur l’heure de modification ; l’échantillonnage du début et de la fin du fichier réduit les faux cache HIT sans devoir relire entièrement le JPEG. En cas de doute, `CACHE CLEAR` force une nouvelle conversion.

**Économie maximale :** avec une seule photo et `single_image_auto_pause=true`, l’alarme RTC est désactivée. Si tu ajoutes ensuite des photos sur la carte sans redémarrer le cadre, elles ne seront pas détectées automatiquement. Pour un album évolutif, mets ce réglage à `false` ; `skip_unchanged` évitera tout de même les rafraîchissements inutiles.

Le cache n’accélère pas l’étape physique de rafraîchissement des pigments ACeP : elle reste obligatoire pour une nouvelle image, lente et gourmande en énergie. Le principal gain provient du **décodage évité** et surtout des **réveils/rafraîchissements supprimés lorsqu’ils sont inutiles**. Aucun chiffre d’autonomie n’est garanti sans mesure sur la carte réelle.

## Batterie et sécurité

La batterie est mesurée via ADC29 (pont diviseur Waveshare ×3). Par défaut :

1. **Au-dessus de 3,4 V** : diaporama normal ; LED ACT allumée pendant l'opération si `activity_led=true`.
2. **De 3,1 à 3,4 V**, sur batterie seule : 5 flashes courts de PWR, tentative d'affichage de la **batterie vide barrée**, puis désactivation de l'alarme et commande de coupure GPIO18.
3. **Sous 3,1 V** : arrêt sans rafraîchissement, pour éviter d'aggraver la décharge.

Important : si la batterie est **réellement épuisée**, il est physiquement impossible de garantir que la dalle pourra encore se rafraîchir. Pour voir le pictogramme, l'alerte doit donc intervenir **avant** la tension critique. Tester et ajuster les seuils à la batterie réelle ; **ne pas décharger volontairement la cellule** pour tester. Brancher l'USB et envoyer `LOWBAT` pour tester l'icône en toute sécurité.

Le cycle sur USB ne coupe pas l'alimentation. Sur batterie, le cadre réveille le RP2040 puis coupe l'alimentation après affichage. La pile bouton CR1220 doit être en état pour conserver l'heure RTC.

## RTC et commandes USB

La première fois, régler l'heure locale depuis l'USB :

```text
SETTIME 2026-10-11 12:30:00
TIME
STATUS
```

Le firmware ne remet pas l'horloge à une date fixe à chaque boot. Sans réseau intégré, il n'y a pas de NTP ou de changement automatique d'heure été/hiver.

| Commande | Effet |
|---|---|
| `HELP`, `STATUS`, `LIST`, `RESCAN`, `SDINFO` | Aide et diagnostics ; `SDINFO` liste la racine et `/pic` ; `RESCAN` recompte les images |
| `CACHE STATS`, `CACHE CLEAR`, `CACHE ON`, `CACHE OFF` | Statistiques, purge des caches, activation volatile |
| `NEXT`, `SHOW 0` | Affiche la prochaine image ou une image par index |
| `TEST`, `CLEAR`, `LOWBAT` | Mire couleur, blanc, pictogramme batterie |
| `TIME`, `SETTIME YYYY-MM-DD HH:MM:SS` | Horloge RTC |
| `ORIENTATION 90`, `INTERVAL 60`, `SHUFFLE ON` | Changements volatils jusqu'au redémarrage |
| `LEDACT ON`/`OFF`, `USB AUTO ON`/`OFF` | Réglages temporaires ACT et déclenchement RTC sur USB |
| `SLEEP`, `BOOTLOADER` | Arrêt batterie ou mode flash UF2 |

## Brochage

| Module | GPIO |
|---|---|
| EPD (DC, CS, SCK, MOSI, RST, BUSY, POWER) | 8, 9, 10, 11, 12, 13, 16 |
| SD (SCK, MOSI, MISO, CS) | 2, 3, 4, 5 |
| PCF85063 RTC (SDA, SCL, INT) | 14, 15, 6 |
| Charge, coupure BAT, bouton NEXT, VBUS, ADC batterie | 17, 18, 19, 24, 29 |
| LED ACT, LED PWR | 25, 26 |

## Références

- Waveshare PhotoPainter original RP2040 : https://github.com/waveshareteam/PhotoPainter
- Décodeur JPEGDEC : https://github.com/bitbank2/JPEGDEC
- Core Arduino-Pico : https://arduino-pico.readthedocs.io/en/latest/platformio.html

Voir `docs/TEST_PLAN.md` pour la validation matérielle. **Cette mise à jour documentaire ne constitue ni une compilation PlatformIO ni une validation sur vraie carte.**

## Diagnostic image absente / console USB (v0.3.3)

Si `SD: ok | images: 0`, cela signifie que la SD est montée **mais qu’aucun nom d’image utilisable n’a été repéré** : le JPEG n’a pas encore été décodé. Depuis le moniteur série, envoyer `SDINFO` (affiche fichiers de la racine et `/pic`), puis `RESCAN`, `LIST` et `NEXT`. Les extensions acceptées sont `.jpg`, `.jpeg`, `.bmp`, `.epd`, `.edp`, insensibles à la casse. Vérifier le vrai système FAT32 (pas exFAT), les noms de dossier, et éviter les sous-dossiers dans `/pic` (scan non récursif). Le scan utilise `/pic`, puis `/PIC`, `/Pic`, puis la racine seulement si aucun fichier image n’a été détecté précédemment.

Dans `platformio.ini` :

```ini
monitor_speed = 115200
monitor_echo = yes
monitor_eol = LF
monitor_filters = send_on_enter
```

Le moniteur PlatformIO ne dispose pas d’une boîte de saisie : cliquer dans le **terminal du moniteur**, taper `SDINFO`, puis Entrée. La commande n’est envoyée qu’après Entrée. Si le firmware indique `RTC: 2053` ou une autre date erronée, utiliser `SETTIME 2026-10-11 14:30:00` (adapter à l'heure locale) puis `TIME`.

**Limitation matérielle** : les JPEG progressifs pleine définition impliquent des mémoires de coefficients considérables. Sur le RP2040 (264 Ko de RAM) le décodeur JPEGDEC 1.8.4 supporte seulement le premier scan DC. L’option `preview` privilégie la compatibilité, mais si l'image doit être nette, la préparation avec PhotoPainter Studio Web (`tools/`) reste recommandée ; elle ne doit être effectuée qu’une fois, car `/cache` élimine les traitements aux affichages suivants.

## v0.3.3 — JPEG sur carte SD

- Corrige les callbacks JPEGDEC : la position `JPEGFILE.iPos` doit être synchronisée lors des lectures et déplacements. Sans cela, le décodeur peut échouer avec certaines images et ses lectures suivantes ne correspondent pas au flux réel.
- Revient explicitement à l'offset 0 après le calcul de la signature de cache.
- Vérifie l'en-tête JPEG `FF D8` et affiche le code d'erreur JPEGDEC et les 12 premiers octets si l'analyse échoue. Un fichier nommé `.jpg` mais contenant du WebP, AVIF ou HTML sera signalé.
- Un fichier non décodable ne bloque plus le bouton NEXT indéfiniment : à la prochaine tentative la galerie passe à l'élément suivant. La photo actuellement affichée n'est pas effacée en cas d'erreur.

**Débogage :** `RESCAN`, `LIST`, `NEXT`, puis copier les lignes `[JPEG]` (dont code d'erreur et signature) si une image ne s'affiche pas. Le message PlatformIO `No new RPxxxx device found yet` ne prouve pas une panne de flash si l'outil affiche ensuite `Verifying Flash: OK` et redémarre l'application.


## v0.4.0 — fichiers natifs et horaires de silence

- `.epd` ou `.edp` : 24 octets d’en-tête PPEF v1 suivis de 192000 octets ACeP-7 4 bits dans l'ordre physique du panneau. Aucun framebuffer intermédiaire ni quantification sur RP2040. Voir [`EPD_FORMAT.md`](EPD_FORMAT.md).
- `settings.conf` ajoute `quiet_hours_start=HH:MM`, `quiet_hours_end=HH:MM` (identiques = désactivé ; passage à minuit supporté) et `battery_calibration=1.000` (0.85..1.15).
- La galerie trie alphabétiquement jusqu’à 512 images. En mode shuffle, un incrément pseudo-aléatoire premier avec le nombre d’images garantit un cycle sans répétition. Ce n’est pas un Fisher–Yates uniforme.
- Le sélecteur AUTO-NEXT reste **un mécanisme matériel de réveil RTC** : le logiciel ne peut pas lire son état.
- `STATUS` affiche les heures de silence et le coefficient de calibration ADC. La mesure n’est pas un système de jauge de batterie à courbe d’autonomie.

[English ↑](#english) · [Français ↑](#francais)
