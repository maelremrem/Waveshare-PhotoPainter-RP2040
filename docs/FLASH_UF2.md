# Compiler et installer le firmware PhotoPainter RP2040

Le firmware est destiné **exclusivement** à la carte Waveshare PhotoPainter RP2040 (F), écran 7,3 pouces ACeP 800×480. Ne pas flasher les cartes RP2350 ou ESP32-S3 avec ce fichier.

## Compilation GitHub Actions

La racine du dépôt contient `platformio.ini` et le workflow `.github/workflows/build-and-publish.yml`.

| Événement | Tests | Compilation RP2040 | Où récupérer le résultat |
| --- | --- | --- | --- |
| Push `main` | Oui | Oui | Studio → Firmware & flash → Dernier build main, après déploiement de Pages ; et artefact du workflow |
| Pull request | Oui | Oui | Artefact du workflow, sans publication sur Pages |
| Tag `vX.Y.Z` | Oui | Oui | GitHub Releases → `PhotoPainter-RP2040.uf2` + `.sha256` |
| Exécution manuelle | Oui | Oui | Artefact du workflow |

Le workflow télécharge PlatformIO, compile avec `pio run -e photopainter_rp2040`, vérifie l'UF2 produit (marqueurs, séquence des blocs, adresse flash et famille RP2040 si annoncée), puis crée une archive de téléchargement. Le dernier UF2 de `main` **n'est pas enregistré dans Git** ; il est ajouté au site lors du déploiement réussi.

**Première mise en ligne :** dans Settings → Pages, sélectionner **GitHub Actions**. Aller dans Actions → **Build firmware and publish Studio** et attendre la réussite des jobs, notamment « Compile RP2040 UF2 » et « Publish Studio ». Puis ouvrir `https://UTILISATEUR.github.io/DEPOT/` → **Firmware & flash**.

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

Sous Fedora si le disque `RPI-RP2` est monté automatiquement :

```bash
cp ~/Téléchargements/PhotoPainter-RP2040-main.uf2 "/run/media/$USER/RPI-RP2/"
```

Adapter le chemin si nécessaire ; vérifier avec `lsblk -f` si le volume ne se monte pas automatiquement. Sous Windows ou macOS, utiliser simplement le gestionnaire de fichiers.

**Sécurité :** s'assurer que la carte cible correspond bien au RP2040, conserver une alimentation USB stable pendant la copie et ne pas flasher un UF2 issu d'un dépôt non fiable. La copie ne modifie pas le contenu de la microSD ni le fichier `settings.conf`.

## Dépannage

- **Aucun `RPI-RP2`** : essayer un autre câble USB de données, un autre port ou la méthode BOOT au branchement ; fermer l'éventuel moniteur série.
- **`No new RPxxxx device found yet`** lors d'un upload automatique : utiliser le mode BOOTSEL puis copier l'UF2 manuellement.
- **L'écran ne change pas** : l'e-paper conserve son image sans alimentation. Contrôler `STATUS`, puis `TEST` ou `NEXT`.
- **Aucun téléchargement dans Studio** : vérifier le succès du workflow sur `main` et du déploiement Pages. Les releases stables exigent un tag et un dépôt GitHub public pour l'API utilisée dans Studio.
- **La compilation échoue** : lire les logs de « Compile RP2040 UF2 » ; le site ne publie pas un nouveau firmware si l'une des vérifications échoue.
