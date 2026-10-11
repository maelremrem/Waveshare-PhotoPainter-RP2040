# PhotoPainter RP2040

**Firmware alternatif pour le cadre photo e-paper Waveshare PhotoPainter RP2040, accompagné de PhotoPainter Studio, un convertisseur d'images accessible dans le navigateur.**

> **Projet d'origine : [Waveshare PhotoPainter — code officiel](https://github.com/waveshareteam/PhotoPainter)**  
> Ce dépôt est une **implémentation communautaire non officielle**. Il reprend le brochage matériel et adapte la séquence d'initialisation de l'écran du projet Waveshare. Il n'est ni développé, ni maintenu, ni approuvé par Waveshare.

## Deux composants dans un seul dépôt

| Composant | Description | Dossier |
| --- | --- | --- |
| **Firmware RP2040** | Diaporama autonome depuis microSD, EPD natif, JPEG/BMP, cache 7 couleurs, RTC, commande NEXT et économie d'énergie | [`src/`](src/), [`include/`](include/) |
| **PhotoPainter Studio Web** | Préparation des photos avec aperçu, 11 méthodes de dithering, modes 7 couleurs et noir/blanc, export EPD/BMP/ZIP et configuration settings.conf | [`tools/`](tools/) |

Le matériel visé est la **Waveshare 7.3inch e-Paper Frame Board / PhotoPainter (F), RP2040**, avec dalle ACeP 7 couleurs de **800 × 480 pixels**. Il ne s'agit **pas** des variantes PhotoPainter RP2350 ou ESP32-S3.

![PhotoPainter Studio : aperçu de l'interface](tools/assets/preview.png)

## PhotoPainter Studio — convertisseur Web

L'application fonctionne entièrement dans le navigateur : les photos restent sur votre ordinateur, sans transfert vers un serveur. Elle propose des corrections photographiques, onze algorithmes de dithering et des modes **sept couleurs** ou **noir et blanc** (photo, doux, contrasté, seuil). L'export **EPD natif** (192024 octets par photo) utilise directement les indices physiques de la dalle et évite toute reconversion sur le RP2040. Un export BMP 24 bits reste disponible pour les anciennes versions du firmware. Le site permet aussi de créer `settings.conf`, de recadrer chaque photo et d'effectuer un diagnostic USB Web Serial (Chromium, connexion explicite).

- **Site GitHub Pages :** après déploiement, `https://VOTRE-PSEUDO.github.io/NOM-DU-DEPOT/`
- **Code source :** [`tools/`](tools/)
- **Documentation du convertisseur :** [`tools/README.md`](tools/README.md)

Pour l'essayer localement :

```bash
cd tools
npm run dev
```

Ouvrir ensuite `http://localhost:8000/` (Node.js requis uniquement pour le serveur local ; aucune installation npm nécessaire).

## Firmware — démarrage rapide

1. Ouvrir la **racine de ce dépôt** dans VS Code avec PlatformIO (le fichier [`platformio.ini`](platformio.ini) doit être à la racine).
2. Sélectionner l'environnement `photopainter_rp2040`, puis **Build** et **Upload**. En cas de problème avec le flash automatique, copier `.pio/build/photopainter_rp2040/firmware.uf2` sur le volume USB `RPI-RP2` en mode BOOTSEL.
3. Formater une carte **microSD FAT32** et copier le contenu de [`sdcard/`](sdcard/) à sa racine.
4. Mettre les photos dans `pic/` et les options dans `settings.conf`.
5. Ouvrir le moniteur série à **115200 bauds** ; utiliser `HELP`, `STATUS`, `SDINFO`, `RESCAN`, `LIST`, `NEXT` pour vérifier le fonctionnement.

Structure de la microSD :

```text
SD:/
├── settings.conf
├── pic/
│   ├── photo001.epd
│   └── photo002.bmp
└── cache/                 # créé automatiquement par le firmware
```

**Méthode conseillée :** préparer les photos dans **PhotoPainter Studio**, choisir **EPD natif** et extraire le dossier `pic/` dans la carte microSD. Le firmware 0.4.0 transfère alors directement les pixels à l'écran, sans décodage ni conversion. Les JPEG et BMP restent pris en charge. Le fichier natif `.epd` (alias `.edp`) est décrit précisément dans [`docs/EPD_FORMAT.md`](docs/EPD_FORMAT.md).

### Principales fonctions

- `.epd`/`.edp` natif 4 bits (CRC32), JPEG baseline et aperçu basse définition de certains JPEG progressifs, BMP 24 bits non compressés.
- Redimensionnement/cadrage en 800 × 480 (ou portrait 480 × 800), rotation 0/90/180/270 degrés.
- Cache persistant sur la SD pour les JPEG/BMP ; les EPD natifs le contournent complètement.
- **NEXT** : changement manuel. **AUTO-NEXT** : interrupteur matériel autorisant les réveils par le RTC sur batterie ; période définie par `refresh_minutes`.
- Gestion des LED ACT/PWR, de la batterie et de l'arrêt matériel entre les rafraîchissements.
- Photos triées par nom et mode pseudo-aléatoire sans répétition dans un cycle. Heures de silence et calibration ADC via `settings.conf`.
- Affichage de batterie vide si l'alimentation est encore suffisante, et protection de seuil critique.

> Les fonctions matérielles, la consommation et la coupure d'alimentation dépendent de la carte réelle. Plusieurs étapes de flash, de lecture SD et de communication série ont été testées sur un RP2040 ; cela **ne constitue pas une validation complète** des fonctions de batterie/RTC/écran.

Les détails de configuration, le brochage, les commandes et les limites des formats sont dans **[la documentation du firmware](docs/FIRMWARE.md)**. Une liste de vérifications est fournie dans [`docs/TEST_PLAN.md`](docs/TEST_PLAN.md).

## Publier ce dépôt sur GitHub

**Un seul dépôt suffit :** le firmware reste à la racine et GitHub Pages publie **uniquement `tools/`**. Le workflow unique [`.github/workflows/build-and-publish.yml`](.github/workflows/build-and-publish.yml) lance les tests du Studio et du format EPD, **compile réellement le firmware RP2040 avec PlatformIO**, puis publie le `.uf2` comme artefact. Sur chaque push réussi dans `main`, l'UF2 est aussi publié directement avec le site GitHub Pages et téléchargeable via l'onglet **Firmware & flash**. Sur un tag `vX.Y.Z`, il devient un asset de la release GitHub sous le nom `PhotoPainter-RP2040.uf2` avec son SHA-256. **La compilation GitHub Actions devra encore être confirmée sur votre dépôt** ; l'environnement de préparation de l'archive ne dispose pas de PlatformIO. Ne considérez pas le firmware téléchargeable avant un build vert.

1. Créer un dépôt GitHub vide (par exemple `PhotoPainter-RP2040`).
2. Extraire **le contenu** de l'archive à la racine du dépôt, sans créer un dossier `Archive/` intermédiaire ; ne pas oublier le dossier caché `.github/`.
3. Pousser les fichiers sur `main`.
4. Dans **Settings → Pages → Build and deployment**, choisir **Source: GitHub Actions**.
5. Vérifier la compilation **Build firmware and publish Studio** dans l’onglet **Actions**, puis ouvrir `https://VOTRE-PSEUDO.github.io/NOM-DU-DEPOT/` et l’onglet **Firmware & flash**.

Premier envoi depuis le dossier extrait :

```bash
git init
git add .
git commit -m "Initial PhotoPainter RP2040 firmware and Studio"
git branch -M main
git remote add origin https://github.com/VOTRE-PSEUDO/NOM-DU-DEPOT.git
git push -u origin main
```

Le dépôt distant doit déjà exister. Si vous avez un dépôt Git avec un historique, n'utilisez pas `git init` ni `git remote add` de nouveau.

### Compilation automatique et versions publiées

- **Push sur `main` :** test Web + tests Python + compilation PlatformIO ; si tout passe, déploiement GitHub Pages avec un fichier `firmware/PhotoPainter-RP2040-main.uf2` téléchargeable dans Studio. Le fichier **n'est pas fourni d'avance** dans le dépôt : GitHub Actions le construit à partir du code.
- **Pull request :** tests et compilation, sans publier de firmware sur le site.
- **Tag `v0.4.1` :** après validation de la compilation et concordance de la version avec `platformio.ini`, création d'une release et ajout de `PhotoPainter-RP2040.uf2` et de son empreinte SHA-256.

Pour publier la première release stable après vérification du matériel :

```bash
git tag v0.4.1
git push origin v0.4.1
```

Pour un autre numéro de version, modifier d'abord `FW_VERSION` dans `platformio.ini` ; le tag et la version doivent correspondre. **Avant de lancer les actions sur le nouveau dépôt, supprimer les anciens fichiers `.github/workflows/pages.yml` et `.github/workflows/firmware.yml` si ceux-ci existent encore.** Le projet fourni utilise désormais un seul workflow.

Voir [le guide flash et compilation](docs/FLASH_UF2.md).

### Tests locaux

```bash
node --test tools/tests/*.test.mjs
python3 tests/test_jpeg_callbacks.py
python3 tests/test_jpeg_fallback.py
python3 tests/test_native_epd.py
python3 tests/test_package_firmware.py
```

Ces tests portent sur le moteur Web et les fonctions de lecture simulées ; ils **ne remplacent pas** une compilation PlatformIO ni les tests matériels.

## Origine, licences et remerciements

**Waveshare Electronics** est à l'origine du matériel PhotoPainter et du [firmware officiel](https://github.com/waveshareteam/PhotoPainter). Le brochage et la séquence d'initialisation ACeP employés ici sont adaptés de son pilote [`EPD_7in3f.c`](https://github.com/waveshareteam/PhotoPainter/blob/master/lib/e-Paper/EPD_7in3f.c). L'en-tête de licence Waveshare est conservé dans `src/Epd.cpp` et reproduit dans [`NOTICE.md`](NOTICE.md), conformément à la notice d'origine.

Dépendances externes du firmware : [Arduino-Pico](https://github.com/earlephilhower/arduino-pico), [JPEGDEC](https://github.com/bitbank2/JPEGDEC) et [TJpg_Decoder](https://github.com/Bodmer/TJpg_Decoder). Elles sont résolues par PlatformIO et conservent leurs licences propres.

**Licence du code original de ce dépôt : [Apache License 2.0](LICENSE)** (voir aussi [Apache-2.0](https://www.apache.org/licenses/LICENSE-2.0)). La licence s'applique au code nouvellement développé pour ce dépôt ; les portions adaptées de Waveshare et les dépendances tierces conservent leurs notices et conditions spécifiques détaillées dans [NOTICE.md](NOTICE.md). La référence au projet officiel de Waveshare est maintenue ci-dessus.

## Évolutions v0.4.0

- **Export EPD natif :** 192 024 octets, en-tête versionné et CRC32, coordonnées physiques écran. Le firmware valide les fichiers **avant** d'alimenter la dalle.
- **Page « Configurer le cadre » :** formulaire validé, import/export `settings.conf`, réglages RTC, cache, batterie et LED.
- **Web Serial :** commandes et synchronisation RTC dans Chromium sur HTTPS/localhost, sans serveur ni transfert de photos.
- **Studio :** zoom, déplacement du cadrage, traitements individuels, PWA et export ZIP `pic/` ; inclure `settings.conf` est optionnel, pour éviter d'écraser par accident la configuration existante.
- **Firmware :** ordre alphabétique stable, parcours pseudo-aléatoire sans répétition, heures de silence, correction ADC de la batterie.
- **Automatisation :** tests JavaScript, build PlatformIO en CI et artefact UF2.

Voir [`docs/ROADMAP.md`](docs/ROADMAP.md) pour les fonctions restant à valider sur matériel et les évolutions futures.

## Évolutions v0.4.1 — firmware téléchargeable

- Compilation automatique du firmware RP2040 depuis GitHub Actions et vérification de la structure UF2.
- UF2 du dernier build réussi publié sur GitHub Pages et accessible depuis Studio.
- Releases stables téléchargeables depuis Studio, avec empreinte SHA-256.
- Nouvelle page **Firmware & flash** et instructions BOOTSEL Windows, macOS et Linux.

