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
- [ ] Tester un JPEG corrompu et un JPEG progressif : message d'erreur, image précédente inchangée, index inchangé.
- [ ] Tester `LIST` avec fichiers cachés, `.jpg`, `.jpeg`, `.bmp` en casse mélangée.
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
- [ ] Tester plusieurs photos : arrêt batterie complet entre réveils et aucune LED d'activité pendant le traitement.
- [ ] Mesurer le courant et l'énergie avec/sans cache sur une même image, et pendant l'arrêt matériel.

## Configuration / alimentation

- [ ] Changer `refresh_minutes`, `shuffle`, `orientation` ; `RESCAN`, `STATUS` reflètent les valeurs.
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

- Le codage JPEG progressif n'est pas garanti par JPEGDEC.
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

Le support progressif n'est pas pleine résolution. La compilation PlatformIO et les essais physiques restent à faire sur le matériel réel.

## v0.3.3 — correction position des fichiers JPEG

1. Tester les JPEG Facebook `.jpg` déjà présents dans `/pic`, y compris progressifs. Vérifier le chargement / cache.
2. Tester un WebP renommé `.jpg` : signalement de la signature sans tentative d'affichage, index suivant sélectionné.
3. Tester un JPEG tronqué : erreur explicite sans écraser la dernière image affichée.
4. Vérifier que NEXT permet d'accéder à une photo valide après un fichier erroné.
5. Vérifier le hit cache après un premier affichage réussi.

## v0.3.4 — alternate JPEG baseline decoder

1. `RESCAN`, `LIST`, then `NEXT` on a Facebook JPG that failed with `error 3` in v0.3.3.
2. If its SOF marker is `0xC0`, expect `[JPEG] Baseline SOF0` followed by `[JPEG/TJPG] Baseline fallback` and an EPD refresh. Repeating the same image should be a cache HIT (provided the cache was saved).
3. If SOF is `0xC2`, expect a clear progressive JPEG message. Use PhotoPainter Studio Web to export BMP or EPD. Full-quality progressive JPEG decoding is not bundled in firmware.
4. Check different JPEG sizes and colors, 90/270 degree rotation, and both image_fit=contain/cover.
5. In the serial monitor, send `SETTIME` alone to print usage, then `SETTIME 2026-10-11 15:40:00`, followed by `TIME`.
6. `python tests/test_jpeg_fallback.py` validates marker scan and TJpg stream callbacks without hardware.

The independent TJpg fallback uses a static workspace and automatically selects 1x, 2x, 4x or 8x downsampling to reduce CPU and SD traffic. Tests with the physical screen are still required.


## Tests spécifiques v0.4.0

- [ ] Placer `sdcard/pic/00000_demo.epd` sur la SD, faire `RESCAN` puis `SHOW n`, vérifier `[EPD] Native frame displayed` et le rendu 7 couleurs.
- [ ] Confirmer que changer `orientation` dans `settings.conf` ne re-tourne pas une image EPD.
- [ ] Corrompre un octet de l’EPD : CRC32 doit empêcher le rafraîchissement.
- [ ] Test batterie + RTC : interruption AUTO-NEXT, heure de réveil en période de pause, reprise à `quiet_hours_end`.
- [ ] Vérifier que les noms triés restent identiques après copie des photos dans un autre ordre.
- [ ] Tester Web Serial `STATUS`, `NEXT`, `SETTIME` via Chrome HTTPS ; USB uniquement, pas de transfert microSD.
- [ ] Mesurer la consommation réelle du RP2040, de la dalle et de la LED ACT pendant conversion, EPD natif, rafraîchissement, et arrêt.
