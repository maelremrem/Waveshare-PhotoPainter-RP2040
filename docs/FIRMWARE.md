# Firmware PhotoPainter RP2040 — documentation technique (v0.3.4)

Dépôt d'origine : [Waveshare PhotoPainter](https://github.com/waveshareteam/PhotoPainter). Ce firmware est une adaptation communautaire non officielle. Retour : [README principal](../README.md).

> **Matériel cible** : PhotoPainter (F) RP2040, ACeP 7 couleurs, 800 × 480. Pas de compatibilité annoncée avec les modèles RP2350 et ESP32-S3.

## Nouveautés

- Photos `.jpg`, `.jpeg` (JPEG baseline, plus aperçu de compatibilité pour les progressifs) et `.bmp` en lecture **directe sur la microSD**.
- **JPG/JPEG et BMP 24 bits redimensionnés** vers 800×480, ou 480×800 pour l’orientation portrait. `image_fit=contain` (bordures blanches) ou `cover` (recadrage central).
- **Cache SD /cache/** au format écran : 4 bits par pixel (192 000 octets + 40 octets d’en-tête), CRC32 et vérification taille/date FAT/échantillon du fichier source.
- En cache HIT, transmission directe de 192 Kio à la dalle, **sans JPEGDEC, redimensionnement ou quantification des couleurs**.
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
2. `Build`, puis `Upload`, port série **115200**. Plateforme Arduino-Pico ; `JPEGDEC 1.8.4` est téléchargée automatiquement par PlatformIO.
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
```

| Clé | Valeurs | Effet |
|---|---|---|
| `orientation` | `0`, `90`, `180`, `270` | Rotation dans le sens horaire ; 90/270 => toile logique portrait 480×800. |
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

`settings.conf` remplace `frame.ini`, mais `frame.ini` et son ancienne clé `interval_minutes` restent lus **en secours** pour les installations existantes. Les clés non reconnues sont ignorées. Les commandes `INTERVAL`, `SHUFFLE`, `ORIENTATION` changent seulement les paramètres en RAM jusqu'au prochain boot.

**Intervalle et switch :** un rafraîchissement complet de cette dalle prend plusieurs dizaines de secondes. L'alarme est programmée `refresh_minutes` après le traitement. Sur batterie, le **switch AUTO-NEXT est un interrupteur matériel du signal RTC** : sur **ON**, l'alarme peut remettre la carte sous tension ; sur **OFF**, l'alarme ne réveille pas la carte. Le firmware ne peut pas lire directement la position du switch (aucun GPIO de sélection documenté). **NEXT** continue de fonctionner dans les deux positions. Sur **USB**, le RP2040 reste alimenté et le signal d'alarme peut être détecté par logiciel sans passer par ce switch ; `usb_auto_next=false` désactive ce contournement, et `usb_auto_next=true` active le cycle périodique même si le switch est sur OFF. Il est donc recommandé de tester le mode ON/OFF **sur batterie**. Avec `single_image_auto_pause=true` et une seule photo, l'alarme est volontairement désactivée, même si AUTO-NEXT est sur ON.

## Formats d'image

- **JPG/JPEG** : JPEG baseline couleur ou niveaux de gris compatibles avec JPEGDEC ; le décodeur lit depuis la SD et convertit les couleurs vers la palette ACeP (noir, blanc, bleu, vert, rouge, orange, jaune). JPEG **progressif** : `progressive_jpeg=preview` autorise une **prévisualisation 1/8 de résolution** (première passe DC seulement), agrandie sur l’écran puis conservée en cache. Ce n’est **pas** un décodage progressif pleine définition et la qualité peut être nettement dégradée. Certains JPEG atypiques resteront non compatibles. Pour une photo de qualité : utiliser l’outil PC pour obtenir un JPEG baseline en 800 × 480.
- **BMP** : RGB **24 bits non compressé**, dimensions quelconques raisonnables (au maximum 16384 pixels par axe) : rééchantillonnage sur la carte en `contain`/`cover`. Les formats indexés, RLE et 32 bits ne sont pas supportés.
- Il n'y a **pas de gestion automatique de l'orientation EXIF** dans le firmware. `orientation` effectue la rotation du contenu pour l'installation du cadre. Pour la rotation EXIF des photos de téléphone, l'PhotoPainter Studio Web (dans `tools/`) peut normaliser l'image.
- Affichage 7 couleurs ; le résultat colorimétrique est forcément plus limité que la photo JPEG d'origine. Pas de mise à jour partielle (ACeP couleur).
- Le scan du dossier suit l'ordre des entrées FAT, pas un tri garanti. Les images ratées ne font pas avancer l'index ; l'image déjà affichée reste en place quand le décodage échoue.

### Préparation recommandée des photos

Le firmware peut décoder certains JPEG baseline, mais les images préconverties donnent de meilleurs résultats et chargent plus vite. Le convertisseur **[PhotoPainter Studio Web](../tools/)** prépare les photos depuis un navigateur, corrige l'orientation EXIF lorsque le navigateur l'applique et propose onze algorithmes de dithering.

Pour exporter vers la microSD : ouvrir PhotoPainter Studio, importer les images, ajuster le rendu, cliquer sur **Exporter tout en ZIP**, puis extraire le dossier `pic/` sur la carte SD. La configuration reste dans `settings.conf` à la racine de la SD.

Les JPEG progressifs ne sont pas décodés en pleine résolution par le firmware : **l'export EPD natif (ou BMP) de PhotoPainter Studio est préférable**. Le mode `preview` des JPEG progressifs intégrés au firmware utilise un aperçu réduit.

## Cache et autonomie (v0.3)

Chaque fichier source est conservé dans `/pic/`. Le RP2040 n’enregistre qu’une image **déjà redimensionnée, tournée et convertie à 7 couleurs**, au format brut **800×480 × 4 bits = 192 000 octets**, avec 40 octets de métadonnées. Enregistrement dans `/cache/xxxxxxxx.epc`, par création d’un fichier temporaire puis renommage en fin d’écriture.

Au réveil :

1. Vérifie la tension avant toute utilisation lourde. La carte SD est montée seulement si la tension n’est pas critique.
2. Charge `settings.conf`, liste `/pic`, identifie la prochaine photo par chemin, taille, date FAT et quelques octets de début/fin.
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

Voir `docs/TEST_PLAN.md` pour la validation matérielle. **Compilation PlatformIO et tests sur vraie carte non effectués dans cet environnement.**

## Diagnostic image absente / console USB (v0.3.3)

Si `SD: ok | images: 0`, cela signifie que la SD est montée **mais qu’aucun nom d’image utilisable n’a été repéré** : le JPEG n’a pas encore été décodé. Depuis le moniteur série, envoyer `SDINFO` (affiche fichiers de la racine et `/pic`), puis `RESCAN`, `LIST` et `NEXT`. Les extensions acceptées sont `.jpg`, `.jpeg`, `.bmp`, insensibles à la casse. Vérifier le vrai système FAT32 (pas exFAT), les noms de dossier, et éviter les sous-dossiers dans `/pic` (scan non récursif). Le scan utilise `/pic`, puis `/PIC`, `/Pic`, puis la racine seulement si aucun fichier image n’a été détecté précédemment.

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
