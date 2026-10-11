# PhotoPainter Studio Web

Une application Web **100 % statique** pour préparer des photos pour le **Waveshare PhotoPainter RP2040 – dalle ACeP 7,3 pouces à 7 couleurs** (800×480). Tout le traitement s'effectue **dans votre navigateur** : les images ne sont jamais envoyées à un serveur.

![Aperçu de PhotoPainter Studio Web](assets/preview.png)

## Fonctions

- Images JPG, JPEG (y compris progressifs si le navigateur les décode), PNG, WEBP et BMP.
- Aperçu original / version convertie en 7 couleurs ou en noir et blanc, en temps réel après modification des paramètres.
- Modes **noir et blanc** : photo tramée, doux, contrasté et seuil/encre (sans tramage), avec points noir/blanc, seuil et inversion. Les gris apparents sont simulés en noir et blanc, jamais exportés comme une troisième teinte.
- Onze méthodes : **aucun tramage, Floyd–Steinberg, Atkinson, Sierra Lite, Burkes, Stucki, Jarvis–Judice–Ninke, Bayer 4×4 / 8×8 / 16×16, bruit réparti**.
- Correspondance des couleurs : OKLab perceptuel, RGB simple ou RGB pondéré.
- Photo : zoom et recadrage à la souris, réglages par photo, orientation, couleur de fond, exposition, gamma, contraste, ombres, hautes lumières, saturation, vibrance, température, teinte, netteté, débruitage léger, niveaux automatiques.
- Préréglages, sauvegarde automatique dans le navigateur et import / export JSON.
- Export **EPD natif** (recommandé) ou BMP RGB24, individuellement ou par archive ZIP `pic/`. Le format EPD nécessite le firmware 0.4.0.
- Comparaison de six algorithmes en une fenêtre.
- Page de configuration du cadre avec import/export `settings.conf` et validation des seuils.
- Onglet **Firmware & flash** : téléchargement du dernier firmware compilé dans GitHub Actions et de la dernière version stable, affichage de la version, du hash SHA-256 et instructions de flash UF2.
- Web Serial pour `STATUS`, `NEXT`, `SETTIME` (Chrome/Chromium, HTTPS ou localhost) ; aucune copie SD par USB.
- PWA installable et fonctionnement hors connexion après mise en cache.
- Aucune dépendance externe, pas de CDN, pas de compilation, pas de backend.

## Déployer sur GitHub Pages (depuis le dépôt complet)

Ce convertisseur est inclus dans le **dépôt monorepo PhotoPainter RP2040** : le firmware et `platformio.ini` restent à la racine, tandis que le site vit dans `tools/`.

Le workflow unique est dans **`../.github/workflows/build-and-publish.yml`**. Il exécute les tests, compile le firmware RP2040 avec PlatformIO et publie **le site `tools/` accompagné du dernier UF2 construit avec succès**. Il peut également publier une release stable sur tag `vX.Y.Z`.

1. Pousser **tout le dépôt** (firmware et `tools/`) sur la branche `main`.
2. Vérifier la présence du répertoire caché `.github/workflows/` (dans la racine du dépôt, et non dans `tools/`).
3. Dans **Settings → Pages → Build and deployment**, sélectionner **GitHub Actions**.
4. Vérifier la réussite du workflow **Build firmware and publish Studio** dans l'onglet **Actions**.
5. Ouvrir `https://VOTRE-PSEUDO.github.io/NOM-DU-DEPOT/`.

Lire le [README principal](../README.md) pour les instructions Git complètes et la référence au projet Waveshare d'origine.

> Le site n'a besoin ni de backend ni de compilation. Les photos restent traitées localement par le navigateur.

## Firmware et flash depuis Studio

L'onglet **Firmware & flash** propose deux téléchargements indépendants :

- **Dernière version stable** : asset `PhotoPainter-RP2040.uf2` de la dernière release GitHub, quand elle existe.
- **Dernier build de `main`** : `firmware/PhotoPainter-RP2040-main.uf2`, automatiquement incorporé au site uniquement après un build réussi. La version, la date et l'empreinte SHA-256 sont affichées à partir de `firmware/build.json` généré par GitHub Actions. Sur un serveur de développement local, le message « aucun UF2 publié » est normal.

La page décrit aussi le flash BOOTSEL : appui BOOT + RUN, volume `RPI-RP2`, copie de l'UF2, redémarrage et commande `STATUS` dans le moniteur série. Le site ne flashe **pas** directement le matériel : une interaction manuelle avec le lecteur USB est nécessaire. Le build exige un runner GitHub Actions connecté à Internet pour télécharger PlatformIO et le cœur RP2040.

## Tester en local

La page utilise un **Web Worker de type ES module** pour ne pas figer l'interface. Elle doit être servie par HTTP(S), pas ouverte en double-cliquant sur `index.html` avec `file://`.

Avec Node.js installé (**sans installer de dépendances**) :

```bash
npm run dev
```

Ou, si Python est déjà installé, uniquement pour servir le site statique :

```bash
python3 -m http.server 8000
```

Puis ouvrir `http://localhost:8000/`.

## Utilisation avec la carte SD

1. Importer les photos dans la page.
2. Choisir les réglages, vérifier le rendu dans l'aperçu.
3. Choisir **EPD natif** (pour le firmware 0.4.0), puis exporter l’image ou tout le ZIP. **BMP** reste disponible pour les anciens firmwares.
4. Pour le ZIP : extraire à la **racine de la microSD** ; il contient un dossier `pic/` prêt à l'emploi. Conserver le `settings.conf` déjà présent sur la carte.
5. Sur le moniteur série du firmware : `RESCAN`, puis `NEXT`.

**EPD natif :** 24 octets d'en-tête + 192000 octets de pixels ACeP quantifiés, toujours dans l'ordre physique 800×480. La rotation Studio y est déjà intégrée, et la valeur `orientation` du firmware n'affecte pas ces fichiers. Le firmware vérifie l'en-tête, les indices et le CRC32 avant de rafraîchir la dalle. Voir [`docs/EPD_FORMAT.md`](../docs/EPD_FORMAT.md).

**BMP :** 24 bits RGB/BGR 800×480 (ou 480×800 portrait), pixels déjà quantifiés. Si vous exportez du BMP en 90°, conservez également `orientation=90` dans `settings.conf`, sinon la rotation sera incorrecte.

Sur la page « Configurer le cadre », exportez `settings.conf` à copier à la racine de la SD. L'option d'inclure ce fichier dans le ZIP est désactivée par défaut pour éviter de remplacer involontairement votre configuration.

**Important** : l'aperçu représente la palette nominale. Le contraste et les nuances réellement affichés dépendent de la dalle, des éclairages et du firmware de l'écran.

## Limites

- Les archives ZIP utilisent le stockage non compressé pour fonctionner sans dépendance et préserver les BMP. Pour beaucoup de grandes photos, la taille du ZIP et l'utilisation mémoire peuvent être importantes.
- Le chargement de TIFF et HEIC dépend du navigateur et n'est pas pris en charge explicitement.
- Le traitement des images est local ; pour une très grande bibliothèque, procéder par lots.
- Les réglages individuels ne sont conservés que tant que les photos sont ouvertes dans l’onglet. Les paramètres globaux sont mémorisés localement dans le navigateur.
- Un algorithme de dithering ne crée pas de couleurs supplémentaires : il simule des mélanges visuels entre les couleurs physiques disponibles (sept en mode couleur, deux en N&B).

## Fichiers du projet

```text
README.md
platformio.ini
src/                       # Firmware RP2040
include/
sdcard/
tools/
├── index.html
├── assets/
│   ├── style.css
│   └── favicon.svg
├── js/
│   ├── app.js
│   ├── engine.js
│   ├── worker.js
│   └── zip.js
├── scripts/dev-server.mjs
├── tests/engine.test.mjs
├── package.json
├── .nojekyll
└── README.md
.github/
└── workflows/pages.yml    # Publie tools/ comme racine du site
```

## Licence

Le code original de PhotoPainter Studio Web est sous **Apache-2.0** (voir [`../LICENSE`](../LICENSE)). Les portions adaptées de Waveshare et les dépendances externes conservent leurs notices dans [`../NOTICE.md`](../NOTICE.md).
