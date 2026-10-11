# Build and install PhotoPainter RP2040 firmware

[English](#english) | [Français](#francais) · [README](../README.md) · [Firmware](FIRMWARE.md) · [UF2](FLASH_UF2.md) · [Roadmap / Feuille de route](ROADMAP.md) · [Tests](TEST_PLAN.md)

<a id="english"></a>

## English

Firmware targets **only** Waveshare PhotoPainter RP2040 (F), 7.3-inch ACeP 800×480. Do not flash RP2350 or ESP32-S3 boards with this file.

## GitHub Actions build

The repository root contains `platformio.ini` and `.github/workflows/build-and-publish.yml`.

| Event | Tests | RP2040 build | Where to get the result |
| --- | --- | --- | --- |
| Push `main` | Yes | Yes | Studio → Firmware & flash → Latest main build, after Pages deployment; also workflow artifact |
| Pull request | Yes | Yes | Workflow artifact, without Pages publication |
| Tag `vX.Y.Z` | Yes | Yes | GitHub Releases → `PhotoPainter-RP2040.uf2` + `.sha256` |
| Manual run | Yes | Yes | Workflow artifact; also Pages when run on `main` |

The workflow downloads PlatformIO, builds with `pio run -e photopainter_rp2040`, verifies the generated UF2 (markers, block sequence, flash address and RP2040 family when declared), then packages the download with build metadata. The latest `main` UF2 **is not committed to Git**; it is added to the site after successful deployment.

**First publication:** in Settings → Pages, select **GitHub Actions**. Open Actions → **Build firmware and publish Studio** and wait for successful jobs, especially “Compile RP2040 UF2” and “Publish Studio + latest successful main build”. Then open `https://USER.github.io/REPOSITORY/` → **Firmware & flash**.

**Create a stable release:** publish a tag only after compilation and hardware tests. For version 0.4.1, run `git tag v0.4.1 && git push origin v0.4.1`. For the next version, update `FW_VERSION` in `platformio.ini` first, then use the matching tag. GitHub Actions rejects mismatches.

## Local build

From the repository root, in VS Code with PlatformIO or a terminal:

```bash
pio run -e photopainter_rp2040
```

Output: `.pio/build/photopainter_rp2040/firmware.uf2`; the first build downloads dependencies. `Upload` can also use picotool when BOOTSEL is accessible, but copying UF2 is the simplest fallback.

## Flash RP2040 using UF2

1. Download a `.uf2` from Studio (**Firmware & flash**), GitHub Releases or a local build.
2. Connect the frame to the computer with a **USB data cable**.
3. Hold **BOOT**, press and release **RUN**, then release **BOOT**. Alternatively, hold BOOT while connecting USB.
4. A USB drive named **`RPI-RP2`** appears. Copy `.uf2` directly to its root: **not to microSD**.
5. The drive disappears after writing; the frame restarts. E-paper may retain its last photo despite flashing.
6. In a 115200-baud serial monitor (or Studio → Configure frame → USB), send `STATUS` to check the firmware version.

Use the file manager on your operating system to copy the downloaded UF2 to the mounted `RPI-RP2` drive. In a shell supporting `cp`, the generic equivalent is:

```bash
cp "<DOWNLOAD_PATH>/PhotoPainter-RP2040-main.uf2" "<RPI-RP2_MOUNT_PATH>/"
```

Replace both placeholders with actual paths; do not type angle brackets literally. Locate the volume with your operating system’s file manager or disk utility if it is not mounted automatically. Do not assume a particular mount directory.

**Safety:** verify the target is RP2040, maintain stable USB power during copying, and do not flash UF2 from an untrusted repository. Copying does not modify microSD contents or `settings.conf`.

## Troubleshooting

- **No `RPI-RP2`:** try another USB data cable, port, or BOOT-at-plug-in method; close any serial monitor.
- **`No new RPxxxx device found yet`** during automatic upload: enter BOOTSEL and copy UF2 manually.
- **Screen unchanged:** e-paper retains its image without power. Check `STATUS`, then `TEST` or `NEXT`.
- **No Studio download:** check successful `main` workflow and Pages deployment. Stable releases require a tag and a public GitHub repository for Studio’s API.
- **Build fails:** read “Compile RP2040 UF2” logs; a new firmware is not published if verification fails.


[Français ↓](#francais) · [Top ↑](#english)

---

<a id="francais"></a>

## Français

# Compiler et installer le firmware PhotoPainter RP2040

Le firmware est destiné **exclusivement** à la carte Waveshare PhotoPainter RP2040 (F), écran 7,3 pouces ACeP 800×480. Ne pas flasher les cartes RP2350 ou ESP32-S3 avec ce fichier.

## Compilation GitHub Actions

La racine du dépôt contient `platformio.ini` et le workflow `.github/workflows/build-and-publish.yml`.

| Événement | Tests | Compilation RP2040 | Où récupérer le résultat |
| --- | --- | --- | --- |
| Push `main` | Oui | Oui | Studio → Firmware & flash → Dernier build main, après déploiement de Pages ; et artefact du workflow |
| Pull request | Oui | Oui | Artefact du workflow, sans publication sur Pages |
| Tag `vX.Y.Z` | Oui | Oui | GitHub Releases → `PhotoPainter-RP2040.uf2` + `.sha256` |
| Exécution manuelle | Oui | Oui | Artefact du workflow ; aussi Pages si exécuté sur `main` |

Le workflow télécharge PlatformIO, compile avec `pio run -e photopainter_rp2040`, vérifie l'UF2 produit (marqueurs, séquence des blocs, adresse flash et famille RP2040 si annoncée), puis prépare le téléchargement et les métadonnées du build. Le dernier UF2 de `main` **n'est pas enregistré dans Git** ; il est ajouté au site lors du déploiement réussi.

**Première mise en ligne :** dans Settings → Pages, sélectionner **GitHub Actions**. Aller dans Actions → **Build firmware and publish Studio** et attendre la réussite des jobs, notamment « Compile RP2040 UF2 » et « Publish Studio + latest successful main build ». Puis ouvrir `https://UTILISATEUR.github.io/DEPOT/` → **Firmware & flash**.

**Créer une release stable :** ne publier un tag qu'après compilation et tests matériels. Pour la version 0.4.1, exécuter `git tag v0.4.1 && git push origin v0.4.1`. Pour la version suivante, mettre d'abord `FW_VERSION` à jour dans `platformio.ini` puis utiliser le tag correspondant. GitHub Actions refusera une discordance.

## Compilation locale

Depuis la racine du dépôt, sous VS Code avec PlatformIO ou en terminal :

```bash
pio run -e photopainter_rp2040
```

Le fichier généré sera `.pio/build/photopainter_rp2040/firmware.uf2` ; la compilation télécharge ses dépendances à la première exécution. `Upload` peut également utiliser picotool si BOOTSEL est accessible, mais la copie UF2 est la méthode de secours la plus simple.

## Flasher le RP2040 avec un fichier UF2

1. Télécharger un `.uf2` depuis Studio (page **Firmware & flash**), la release GitHub ou le build local.
2. Relier le cadre au PC avec un **câble USB de données**.
3. Maintenir **BOOT**, appuyer puis relâcher **RUN**, puis relâcher **BOOT**. Alternative : tenir BOOT tout en branchant l'USB.
4. Un disque USB **`RPI-RP2`** apparaît. Copier le `.uf2` directement à sa racine : **ne pas copier sur la microSD**.
5. Le disque disparaît quand le firmware a été écrit ; le cadre redémarre. L'écran e-paper peut conserver la dernière photo malgré le flash.
6. Dans un moniteur série 115200 bauds (ou Studio → Configurer le cadre → USB), envoyer `STATUS` pour vérifier la version du firmware.

Utiliser le gestionnaire de fichiers de votre système pour copier l’UF2 téléchargé sur le volume monté `RPI-RP2`. Dans un shell prenant en charge `cp`, l’équivalent générique est :

```bash
cp "<DOWNLOAD_PATH>/PhotoPainter-RP2040-main.uf2" "<RPI-RP2_MOUNT_PATH>/"
```

Remplacer les deux paramètres par les chemins réels ; ne pas saisir littéralement les chevrons. Si le volume ne se monte pas automatiquement, le repérer avec le gestionnaire de fichiers ou l’utilitaire de disques de votre système. Ne présumer aucun dossier de montage particulier.

**Sécurité :** s'assurer que la carte cible correspond bien au RP2040, conserver une alimentation USB stable pendant la copie et ne pas flasher un UF2 issu d'un dépôt non fiable. La copie ne modifie pas le contenu de la microSD ni le fichier `settings.conf`.

## Dépannage

- **Aucun `RPI-RP2`** : essayer un autre câble USB de données, un autre port ou la méthode BOOT au branchement ; fermer l'éventuel moniteur série.
- **`No new RPxxxx device found yet`** lors d'un upload automatique : utiliser le mode BOOTSEL puis copier l'UF2 manuellement.
- **L'écran ne change pas** : l'e-paper conserve son image sans alimentation. Contrôler `STATUS`, puis `TEST` ou `NEXT`.
- **Aucun téléchargement dans Studio** : vérifier le succès du workflow sur `main` et du déploiement Pages. Les releases stables exigent un tag et un dépôt GitHub public pour l'API utilisée dans Studio.
- **La compilation échoue** : lire les logs de « Compile RP2040 UF2 » ; le site ne publie pas un nouveau firmware si l'une des vérifications échoue.

[English ↑](#english) · [Français ↑](#francais)
