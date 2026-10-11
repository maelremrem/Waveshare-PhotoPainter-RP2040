# RP2040 / PhotoPainter (F) validation

[English](#english) | [Français](#francais) · [README](../README.md) · [Firmware](FIRMWARE.md) · [UF2](FLASH_UF2.md) · [Roadmap / Feuille de route](ROADMAP.md) · [Tests](TEST_PLAN.md)

<a id="english"></a>

## English

## Preparation

- [ ] Verify RP2040 and seven-color ACeP (F) display on the board.
- [ ] Build PlatformIO without errors; check JPEGDEC 1.8.4 availability.
- [ ] Back up the original SD, flash UF2 over USB.
- [ ] Check `STATUS`, battery voltage, RTC and SD detection.
- [ ] Set RTC, check `TIME` after restart and after 5 minutes.

## Photos

- [ ] Test `TEST`, then `CLEAR`, without BUSY timeout.
- [ ] Baseline JPG 800×480, 1920×1080 and large format from SD; `NEXT` works.
- [ ] Portrait JPEG, `orientation=90`, then `orientation=270` in settings.conf.
- [ ] BMP 800×480 (0° and 180°), BMP 480×800 (90° and 270°), BMP 4000×3000 (downscale), BMP 200×150 (upscale).
- [ ] Test `image_fit=contain` and `cover` for JPEG **and BMP** with different photo aspect ratios.
- [ ] Test corrupt JPEG and progressive JPEG with `progressive_jpeg=reject`: explicit error, previous image unchanged; with multiple photos, next attempt skips the failed file. With `preview`, supported progressive files should instead display a reduced preview.
- [ ] Test `LIST` with hidden files and mixed-case `.jpg`, `.jpeg`, `.bmp`, `.epd`, `.edp`.
- [ ] Check conversion time with/without RAM framebuffer and free SD space (>192 kB).
- [ ] Check no stuck render.tmp after normal operation.

## Cache and energy savings

- [ ] With USB connected, display JPEG; check `/cache/*.epc`: 192040-byte size and `CACHE STATS`.
- [ ] `CACHE CLEAR`, `NEXT` => MISS / STORED; return to this photo after another => HIT without JPEG decoding.
- [ ] Redisplay same photo with `skip_unchanged=true` => no EPD command or refresh.
- [ ] Change 1 byte at JPEG start/end while retaining size/date: new cache (sample changed).
- [ ] Change `orientation` or `image_fit` => cache regenerated.
- [ ] Corrupt/truncate `.epc`: CRC or size detected, regeneration, no partial refresh.
- [ ] Cut power during cache generation: old cache intact or automatic reconstruction; no incomplete cache accepted as valid.
- [ ] Check near `cache_write_min_v`: no cache creation with low battery and no USB.
- [ ] One photo, `single_image_auto_pause=true`: no automatic RTC wakeup; manual recovery works.
- [ ] Multiple photos: complete battery shutdown between wakeups; ACT off during shutdown and during processing only if `activity_led=false`.
- [ ] Measure current/energy with and without cache on the same image, and during hardware shutdown.

## Configuration / power

- [ ] Change `refresh_minutes`, `shuffle`, `orientation` in settings.conf, then `RESCAN` or reboot; `STATUS` reflects values. `RESCAN` rescans images and reloads configuration from defaults plus the SD file. Serial commands change RAM settings only until reboot or `RESCAN`.
- [ ] Disable settings.conf, then test legacy frame.ini compatibility.
- [ ] Verify GPIO19: NEXT advances on USB and battery with switch OFF or ON.
- [ ] ACT LED (GPIO25) on while loading/converting on battery AND USB; off after display; always off with `activity_led=false`.
- [ ] PWR LED (GPIO26): on with USB and active charging, off after charge completion, five flashes at noncritical low threshold.
- [ ] Battery, 2+ photos, AUTO-NEXT OFF: no periodic wakeup; manual NEXT available.
- [ ] Battery, 2+ photos, AUTO-NEXT ON: wake after `refresh_minutes`, next photo, hardware shutdown; NEXT remains available.
- [ ] USB: `usb_auto_next=false` => no automatic advance; `usb_auto_next=true` => periodic software cycling independent of switch.
- [ ] `single_image_auto_pause=true`: one image means no automatic wake even with switch ON.
- [ ] `LOWBAT` on USB: full-screen crossed-out empty battery, no corruption.
- [ ] Test battery threshold **through ADC readings or instrumented simulation**, never deliberate deep discharge.
- [ ] Slightly below `low_battery_v` but above `critical_cutoff_v`: warning, disabled alarm and cutoff command.
- [ ] Below `critical_cutoff_v`: program does not attempt panel refresh.
- [ ] Check hardware power really turns off and RTC retains time.

## Known limitations

- JPEGDEC does not guarantee progressive JPEG compatibility; supported preview is not full quality.
- On-board EXIF rotation is not read; normalize with the tool if needed.
- SD can act as temporary framebuffer when 192 kB contiguous RP2040 RAM is unavailable.
- The (F) panel has no reliable partial refresh.
- An icon cannot be displayed if battery voltage is already too low for the EPD sequence.

## v0.3.2 — gallery/JPEG/console

1. Put `photo.jpg` in `/pic`, then `RESCAN`, `SDINFO`, `LIST`. Verify `images >= 1`.
2. Test `.JPG`, `.JPEG`, and files directly at root without `/pic`.
3. Test baseline JPEG 800x480 and progressive JPEG 1600x900 with `progressive_jpeg=preview`: progressive must provide only an **enlarged 1/8 preview**.
4. Use the computer tool to reconvert progressive to baseline JPEG, delete old cache, check sharp display.
5. Check `monitor_echo=yes` + `send_on_enter`: terminal shows typed letters and `STATUS` responds after Enter.
6. Check all three voltages displayed to 2 decimal places; set RTC with `SETTIME` if necessary.

Progressive support is not full resolution. This documentation update does not establish PlatformIO build success or physical validation; verify both before release.

## v0.3.3 — JPEG file-position correction

1. Test Facebook `.jpg` files already in `/pic`, including progressive files. Check loading/cache.
2. Test WebP renamed `.jpg`: signature reported without display attempt; next index selected.
3. Test truncated JPEG: explicit error without overwriting last displayed image.
4. Check NEXT reaches a valid photo after an erroneous file.
5. Check cache HIT after first successful display.

## v0.3.4 — alternate JPEG baseline decoder

1. `RESCAN`, `LIST`, then `NEXT` on a Facebook JPG that failed with `error 3` in v0.3.3.
2. If its SOF marker is `0xC0`, expect `[JPEG] Baseline SOF0` followed by `[JPEG/TJPG] Baseline fallback` and an EPD refresh. Repeating the same image should be a cache HIT (provided the cache was saved).
3. If SOF is `0xC2`, expect a clear progressive JPEG message. Use PhotoPainter Studio Web to export BMP or EPD. Full-quality progressive JPEG decoding is not bundled in firmware.
4. Check different JPEG sizes and colors, 90/270 degree rotation, and both image_fit=contain/cover.
5. In the serial monitor, send `SETTIME` alone to print usage, then `SETTIME 2026-10-11 15:40:00`, followed by `TIME`.
6. `python tests/test_jpeg_fallback.py` validates marker scan and TJpg stream callbacks without hardware.

The independent TJpg fallback uses a static workspace and automatically selects 1x, 2x, 4x or 8x downsampling to reduce CPU and SD traffic. Tests with the physical screen are still required.

## v0.4.0-specific tests

- [ ] Put `sdcard/pic/00000_demo.epd` on SD, run `RESCAN`, then `SHOW n`, check `[EPD] Native frame displayed` and seven-color rendering.
- [ ] Confirm changing `orientation` in `settings.conf` does not rotate EPD again.
- [ ] Corrupt an EPD byte: CRC32 must prevent refresh.
- [ ] Battery + RTC: AUTO-NEXT interrupt, wake time during quiet hours, resume at `quiet_hours_end`.
- [ ] Check sorted names remain identical after copying photos in a different order.
- [ ] Test Web Serial `STATUS`, `NEXT`, `SETTIME` through Chrome HTTPS; USB only, no microSD transfer.
- [ ] Measure real RP2040, panel and ACT LED consumption during conversion, native EPD, refresh and shutdown.


[Français ↓](#francais) · [Top ↑](#english)

---

<a id="francais"></a>

## Français

# Validation RP2040 / PhotoPainter (F)

## Préparation

- [ ] Vérifier RP2040 et écran ACeP 7 couleurs (F) sur la carte.
- [ ] Compiler PlatformIO sans erreur ; vérifier la disponibilité de JPEGDEC 1.8.4.
- [ ] Sauvegarder la carte SD d'origine, flasher UF2 sur USB.
- [ ] Vérifier `STATUS`, tension batterie, RTC et détection SD.
- [ ] Régler RTC, contrôler `TIME` après redémarrage et après 5 minutes.

## Photos

- [ ] Tester `TEST` puis `CLEAR`, pas de timeout BUSY.
- [ ] JPG baseline 800×480, 1920×1080 et grand format depuis SD ; `NEXT` fonctionne.
- [ ] JPEG portrait, `orientation=90` puis `orientation=270` dans settings.conf.
- [ ] BMP 800×480 (0° et 180°), BMP 480×800 (90° et 270°), BMP 4000×3000 (downscale), BMP 200×150 (upscale).
- [ ] Tester `image_fit=contain` et `cover` pour JPEG **et BMP** avec formats de photo différents.
- [ ] Tester un JPEG corrompu et un JPEG progressif avec `progressive_jpeg=reject` : erreur explicite, image précédente inchangée ; avec plusieurs photos, la prochaine tentative saute le fichier erroné. Avec `preview`, un progressif compatible doit afficher un aperçu réduit.
- [ ] Tester `LIST` avec fichiers cachés, `.jpg`, `.jpeg`, `.bmp`, `.epd`, `.edp` en casse mélangée.
- [ ] Vérifier temps de conversion avec / sans RAM framebuffer et l'espace disponible (>192 Ko) sur SD.
- [ ] Vérifier absence de fichier render.tmp bloqué après fonctionnement normal.

## Cache et économie d'énergie

- [ ] Avec USB branché, montrer un JPEG ; vérifier `/cache/*.epc` : taille 192040 octets et `CACHE STATS`.
- [ ] `CACHE CLEAR`, `NEXT` => MISS / STORED ; remettre sur la même photo après une autre => HIT sans décodage JPEG.
- [ ] Réafficher la même photo avec `skip_unchanged=true` => aucune commande EPD ni rafraîchissement.
- [ ] Modifier 1 octet au début/fin d'un JPEG conservant même taille/date : nouveau cache (échantillon modifié).
- [ ] Modifier `orientation` ou `image_fit` => cache régénéré.
- [ ] Corrompre/tronquer un `.epc` : CRC ou taille détectée, régénération, pas de rafraîchissement partiel.
- [ ] Couper le courant pendant la génération du cache : ancien cache intact ou reconstruction automatique, aucun cache incomplet considéré valide.
- [ ] Vérifier le comportement proche du seuil `cache_write_min_v` : pas de création de cache si batterie faible et USB absent.
- [ ] Tester une seule photo et `single_image_auto_pause=true` : pas de réveil RTC automatique, reprise manuelle possible.
- [ ] Tester plusieurs photos : arrêt batterie complet entre réveils ; ACT éteinte pendant l’arrêt et pendant le traitement seulement si `activity_led=false`.
- [ ] Mesurer le courant et l'énergie avec/sans cache sur une même image, et pendant l'arrêt matériel.

## Configuration / alimentation

- [ ] Changer `refresh_minutes`, `shuffle`, `orientation` dans settings.conf puis `RESCAN` ou redémarrer ; `STATUS` reflète les valeurs. `RESCAN` rescane les images et recharge les réglages à partir des valeurs par défaut et du fichier SD. Les commandes série ne changent que les valeurs en RAM jusqu’au redémarrage ou à `RESCAN`.
- [ ] Désactiver settings.conf puis essayer l'ancien frame.ini (compatibilité).
- [ ] Vérifier GPIO19 : NEXT passe à l'image suivante sur USB et sur batterie, switch OFF ou ON.
- [ ] Vérifier LED ACT (GPIO25) allumée pendant le chargement et la conversion sur batterie ET USB ; éteinte dès la fin de l'affichage ; éteinte en permanence quand `activity_led=false`.
- [ ] Vérifier LED PWR (GPIO26) : allumée USB et charge active, éteinte en fin de charge, 5 flashes au seuil bas non critique.
- [ ] Sur batterie avec 2+ photos, switch AUTO-NEXT OFF : aucun réveil périodique ; NEXT manuel disponible.
- [ ] Sur batterie avec 2+ photos, switch AUTO-NEXT ON : réveil à `refresh_minutes`, photo suivante, arrêt matériel ; NEXT toujours disponible.
- [ ] Sur USB : `usb_auto_next=false` => pas de passage automatique ; `usb_auto_next=true` => défilement logiciel périodique indépendant du switch.
- [ ] Vérifier `single_image_auto_pause=true` : avec une seule image, aucun réveil automatique même switch ON.
- [ ] `LOWBAT` sur USB : pictogramme batterie vide barrée, tout l'écran, zéro corruption.
- [ ] Tester seuil batterie **par lecture ADC ou simulation instrumentée**, jamais par décharge profonde volontaire.
- [ ] À une tension légèrement inférieure à `low_battery_v` mais supérieure à `critical_cutoff_v`, contrôler le warning, l'alarme désactivée et la commande de coupure.
- [ ] Sous `critical_cutoff_v`, vérifier que le programme ne tente pas de rafraîchir la dalle.
- [ ] Contrôler si l'alimentation matérielle se coupe réellement et si le RTC continue à tenir l'heure.

## Limitations connues

- La compatibilité JPEG progressif n’est pas garantie par JPEGDEC ; l’aperçu compatible n’est pas en pleine qualité.
- La rotation EXIF n'est pas lue sur la carte ; la normaliser avec l'outil si nécessaire.
- La SD peut servir de framebuffer temporaire si 192 Ko contigus sont indisponibles dans la RAM du RP2040.
- La dalle (F) n'offre pas de rafraîchissement partiel fiable.
- Impossible d'afficher une icône si la batterie est déjà trop faible pour lancer la séquence EPD.

## v0.3.2 – galerie/JPEG/console

1. Mettre `photo.jpg` dans `/pic`, puis `RESCAN`, `SDINFO`, `LIST`. Vérifier que `images >= 1`.
2. Tester des fichiers `.JPG` et `.JPEG`, et des fichiers directement à la racine sans `/pic`.
3. Tester un JPEG baseline 800x480 et un JPEG progressif 1600x900 avec `progressive_jpeg=preview` : la version progressive ne doit fournir qu’un **aperçu agrandi 1/8**.
4. Utiliser l'outil PC pour reconvertir ce progressif en JPEG baseline, supprimer son ancien cache, vérifier l'affichage net.
5. Vérifier `monitor_echo=yes` + `send_on_enter` : les lettres s'affichent dans le terminal et `STATUS` répond à Entrée.
6. Vérifier l'affichage des trois tensions à 2 décimales et régler le RTC avec `SETTIME` si nécessaire.

Le support progressif n'est pas pleine résolution. Cette mise à jour documentaire ne prouve ni une compilation PlatformIO réussie ni une validation physique ; vérifier les deux avant publication.

## v0.3.3 — correction position des fichiers JPEG

1. Tester les JPEG Facebook `.jpg` déjà présents dans `/pic`, y compris progressifs. Vérifier le chargement / cache.
2. Tester un WebP renommé `.jpg` : signalement de la signature sans tentative d'affichage, index suivant sélectionné.
3. Tester un JPEG tronqué : erreur explicite sans écraser la dernière image affichée.
4. Vérifier que NEXT permet d'accéder à une photo valide après un fichier erroné.
5. Vérifier le hit cache après un premier affichage réussi.

## v0.3.4 — décodeur JPEG baseline alternatif

1. `RESCAN`, `LIST`, puis `NEXT` sur un JPG Facebook ayant échoué avec `error 3` en v0.3.3.
2. Si le marqueur SOF est `0xC0`, attendre `[JPEG] Baseline SOF0`, puis `[JPEG/TJPG] Baseline fallback` et un rafraîchissement EPD. Réafficher la même image doit donner un cache HIT (si le cache a été enregistré).
3. Si SOF est `0xC2`, attendre un message JPEG progressif explicite. Exporter BMP ou EPD avec PhotoPainter Studio Web. Le décodage progressif pleine qualité n’est pas inclus dans le firmware.
4. Vérifier plusieurs tailles/couleurs JPEG, rotation 90/270 degrés et les deux modes image_fit=contain/cover.
5. Dans le moniteur série, envoyer `SETTIME` seul pour afficher l’usage, puis `SETTIME 2026-10-11 15:40:00`, puis `TIME`.
6. `python tests/test_jpeg_fallback.py` valide le scan des marqueurs et les callbacks de flux TJpg sans matériel.

Le secours TJpg indépendant utilise un espace de travail statique et choisit automatiquement un sous-échantillonnage 1x, 2x, 4x ou 8x pour réduire le calcul et les accès SD. Les tests sur l’écran physique restent nécessaires.

## Tests spécifiques v0.4.0

- [ ] Placer `sdcard/pic/00000_demo.epd` sur la SD, faire `RESCAN` puis `SHOW n`, vérifier `[EPD] Native frame displayed` et le rendu 7 couleurs.
- [ ] Confirmer que changer `orientation` dans `settings.conf` ne re-tourne pas une image EPD.
- [ ] Corrompre un octet de l’EPD : CRC32 doit empêcher le rafraîchissement.
- [ ] Test batterie + RTC : interruption AUTO-NEXT, heure de réveil en période de pause, reprise à `quiet_hours_end`.
- [ ] Vérifier que les noms triés restent identiques après copie des photos dans un autre ordre.
- [ ] Tester Web Serial `STATUS`, `NEXT`, `SETTIME` via Chrome HTTPS ; USB uniquement, pas de transfert microSD.
- [ ] Mesurer la consommation réelle du RP2040, de la dalle et de la LED ACT pendant conversion, EPD natif, rafraîchissement, et arrêt.

[English ↑](#english) · [Français ↑](#francais)
