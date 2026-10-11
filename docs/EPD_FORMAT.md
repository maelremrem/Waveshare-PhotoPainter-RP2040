# PhotoPainter EPD native frame / Image native EPD PhotoPainter — PPEF v1

[English](#english) | [Français](#français)

## English

The `.epd` extension identifies a ready-to-display PhotoPainter 7-color frame. Firmware also accepts `.edp` as a spelling alias. The format is **not** the undocumented Waveshare proprietary format. It is explicitly defined here for this project. All integers are little-endian.

| Offset | Bytes | Field | Value |
|---|---:|---|---|
| 0 | 4 | Signature | ASCII `PPEF` |
| 4 | 2 | Version | `1` |
| 6 | 2 | Physical width | `800` |
| 8 | 2 | Physical height | `480` |
| 10 | 1 | Pixel format | `1` (4-bit ACeP-7) |
| 11 | 1 | Flags | `0` |
| 12 | 4 | Payload length | `192000` |
| 16 | 4 | CRC32 | IEEE CRC32 of payload, starting at offset 24 |
| 20 | 4 | Reserved | `0` |
| 24 | 192000 | Payload | packed pixels, row-major |

**One packed byte is two pixels:** high nibble is even physical X, low nibble is odd physical X. Rows proceed left to right, top to bottom, no padding. Valid palette indices: `0=black`, `1=white`, `2=green`, `3=blue`, `4=red`, `5=yellow`, `6=orange`. The value `7` and all other indices are invalid.

Total size is **192024 bytes**. The payload is **always landscape 800×480 in the physical panel coordinate system**, even when the user's source photo is portrait. PhotoPainter Studio pre-rotates the quantized output into that physical order. The firmware **must not rotate native frames again** when `orientation` in `settings.conf` changes. That setting affects only JPEG and BMP sources.

### Reader security and power behavior

Before powering the panel, the RP2040 checks the signature, header version/format, declared length, exact file size, palette values and CRC32. It then seeks back to offset 24 and streams rows to EPD RAM, refreshing only after a complete read. This adds one sequential SD validation pass but avoids a potentially long erroneous screen refresh on a truncated or corrupt file. The CRC is not a cryptographic signature.

Original BMP/JPEG paths and their `.epc` caches are preserved. Native EPD files are **not additionally cached**. The existing last-displayed marker uses the file's length, timestamp and sampled file bytes to skip unnecessary repeats.

### Example microSD

```text
SD:/
  settings.conf
  pic/
    001_family.epd
    002_landscape.epd
  cache/  # only for BMP/JPEG conversions
```

### Compatibility

Introduced with firmware **0.4.0** and Studio v0.4.0. Earlier firmware releases do not recognize the format. Export BMP instead if using earlier firmware.

## Français

L'extension `.epd` désigne une image PhotoPainter à 7 couleurs prête à afficher. Le firmware accepte également `.edp` comme variante orthographique. Ce format **n'est pas** le format propriétaire Waveshare non documenté. Il est explicitement défini ici pour ce projet. Tous les entiers sont en petit-boutiste (little-endian).

| Décalage | Octets | Champ | Valeur |
|---|---:|---|---|
| 0 | 4 | Signature | ASCII `PPEF` |
| 4 | 2 | Version | `1` |
| 6 | 2 | Largeur physique | `800` |
| 8 | 2 | Hauteur physique | `480` |
| 10 | 1 | Format de pixel | `1` (ACeP-7 4 bits) |
| 11 | 1 | Drapeaux | `0` |
| 12 | 4 | Longueur des données utiles | `192000` |
| 16 | 4 | CRC32 | CRC32 IEEE des données utiles, à partir du décalage 24 |
| 20 | 4 | Réservé | `0` |
| 24 | 192000 | Données utiles | pixels empaquetés, ligne par ligne |

**Un octet empaqueté contient deux pixels :** le quartet de poids fort correspond à une coordonnée X physique paire, le quartet de poids faible à une coordonnée X physique impaire. Les lignes sont parcourues de gauche à droite et de haut en bas, sans remplissage. Indices de palette valides : `0=noir`, `1=blanc`, `2=vert`, `3=bleu`, `4=rouge`, `5=jaune`, `6=orange`. La valeur `7` et tous les autres indices sont invalides.

La taille totale est de **192024 octets**. Les données utiles sont **toujours en paysage 800×480 dans le système de coordonnées physiques de la dalle**, même si la photo source de l'utilisateur est en portrait. PhotoPainter Studio effectue à l'avance la rotation de la sortie quantifiée pour respecter cet ordre physique. Le firmware **ne doit pas effectuer une nouvelle rotation des images natives** lorsque `orientation` change dans `settings.conf`. Ce paramètre ne concerne que les sources JPEG et BMP.

### Sécurité du lecteur et gestion de l'alimentation

Avant d'alimenter la dalle, le RP2040 vérifie la signature, la version et le format de l'en-tête, la longueur déclarée, la taille exacte du fichier, les valeurs de palette et le CRC32. Il revient ensuite au décalage 24 et transmet les lignes à la RAM de l'EPD, en ne déclenchant le rafraîchissement qu'après une lecture complète. Cela ajoute une passe de validation séquentielle sur la carte SD, mais évite un rafraîchissement erroné potentiellement long en cas de fichier tronqué ou corrompu. Le CRC n'est pas une signature cryptographique.

Les chemins de traitement BMP/JPEG d'origine et leurs caches `.epc` sont conservés. Les fichiers EPD natifs **ne font pas l'objet d'une mise en cache supplémentaire**. Le marqueur existant de la dernière image affichée utilise la longueur du fichier, son horodatage et des octets échantillonnés du fichier pour éviter les répétitions inutiles.

### Exemple de carte microSD

```text
SD:/
  settings.conf
  pic/
    001_family.epd
    002_landscape.epd
  cache/  # uniquement pour les conversions BMP/JPEG
```

### Compatibilité

Introduit avec le firmware **0.4.0** et Studio v0.4.0. Les versions antérieures du firmware ne reconnaissent pas ce format. Exportez plutôt en BMP si vous utilisez un firmware antérieur.
