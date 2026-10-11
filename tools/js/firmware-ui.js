/** Firmware downloads and BOOTSEL instructions for the RP2040 (F) frame. */
const $ = selector => document.querySelector(selector);
const REPO = /^[a-z\d_.-]+\/[a-z\d_.-]+$/i;
const HASH = /^[a-f\d]{64}$/i;

export function repositoryFromSite(urlString) {
  let url;
  try { url = new URL(urlString); } catch { return ''; }
  if (!url.hostname.endsWith('.github.io')) return '';
  const owner = url.hostname.slice(0, -'.github.io'.length);
  if (!owner || !/^[a-z\d-]+$/i.test(owner)) return '';
  const project = url.pathname.split('/').filter(Boolean)[0] || `${owner}.github.io`;
  const repo = `${owner}/${project}`;
  return REPO.test(repo) ? repo : '';
}

export function verifiedBuild(metadata) {
  if (!metadata || typeof metadata !== 'object' || !REPO.test(metadata.repository || '')) return null;
  if (!/^\d+(\.\d+){1,3}$/.test(metadata.version || '')) return null;
  if (metadata.file !== 'PhotoPainter-RP2040-main.uf2') return null;
  if (!Number.isInteger(metadata.bytes) || metadata.bytes < 512 || metadata.bytes % 512) return null;
  if (!HASH.test(metadata.sha256 || '')) return null;
  return metadata;
}

export function latestReleaseAsset(release, repository) {
  if (!REPO.test(repository) || !release || !Array.isArray(release.assets)) return null;
  const asset = release.assets.find(a => a?.name === 'PhotoPainter-RP2040.uf2');
  if (!asset || typeof asset.browser_download_url !== 'string') return null;
  try {
    const url = new URL(asset.browser_download_url);
    const prefix = `/${repository}/releases/download/`;
    if (url.protocol !== 'https:' || url.hostname !== 'github.com' || !url.pathname.startsWith(prefix)) return null;
    if (!url.pathname.endsWith('/PhotoPainter-RP2040.uf2')) return null;
    return {url: url.href, tag: String(release.tag_name || ''), size: asset.size || null};
  } catch { return null; }
}

function showLink(anchor, href, label) {
  anchor.href = href;
  anchor.textContent = label;
  anchor.hidden = false;
}
function setText(id, value) { $(id).textContent = value; }
function prettyDate(date) {
  const d = new Date(date);
  return Number.isFinite(d.getTime()) ? d.toLocaleString('fr-FR', {dateStyle: 'medium', timeStyle: 'short'}) : '—';
}

export function startFirmwareUI() {
  let refreshing = false;
  const stableButton = $('#fwStableDownload');
  const devButton = $('#fwDevDownload');
  const releasesLink = $('#fwReleasesLink');
  const actionsLink = $('#fwActionsLink');
  const refresh = $('#fwRefresh');
  const localRepo = repositoryFromSite(location.href);

  async function update() {
    if (refreshing) return;
    refreshing = true;
    refresh.disabled = true;
    stableButton.hidden = true;
    devButton.hidden = true;
    setText('#fwBuildStatus', 'Recherche du dernier build…');
    setText('#fwReleaseStatus', 'Recherche de la dernière release…');
    try {
      let data = null;
      try {
        const res = await fetch(new URL('../firmware/build.json', import.meta.url), {cache:'no-store'});
        if (res.ok) data = await res.json();
      } catch { /* The local dev server may not have a compiled UF2. */ }
      const build = verifiedBuild(data);
      const repo = (data && REPO.test(data.repository || '') ? data.repository : localRepo);
      if (repo) {
        showLink(releasesLink, `https://github.com/${repo}/releases`, 'Toutes les releases GitHub ↗');
        showLink(actionsLink, `https://github.com/${repo}/actions`, 'Voir les compilations GitHub ↗');
      } else {
        releasesLink.hidden = true;
        actionsLink.hidden = true;
      }

      if (build) {
        const url = new URL('../firmware/PhotoPainter-RP2040-main.uf2', import.meta.url);
        url.searchParams.set('v', build.sha256.slice(0, 16));
        showLink(devButton, url.href, '↓ Télécharger le firmware (main)');
        devButton.download = 'PhotoPainter-RP2040-main.uf2';
        setText('#fwBuildStatus', `Version ${build.version} · ${prettyDate(build.built_at)} · ${(build.bytes / 1024).toFixed(0)} Kio`);
        setText('#fwBuildCommit', build.commit ? build.commit.slice(0,12) : '—');
        setText('#fwBuildChecksum', build.sha256);
      } else {
        setText('#fwBuildStatus', 'Aucun UF2 publié sur ce site. La première compilation GitHub Actions doit réussir.');
        setText('#fwBuildCommit', '—');
        setText('#fwBuildChecksum', '—');
      }

      if (!repo) {
        setText('#fwReleaseStatus', 'Dépôt GitHub non identifié. Publiez ce site avec le workflow du projet pour activer la recherche des releases.');
        return;
      }
      try {
        const controller = new AbortController();
        const timer = setTimeout(() => controller.abort(), 7000);
        let response;
        try { response = await fetch(`https://api.github.com/repos/${repo}/releases/latest`, {headers:{Accept:'application/vnd.github+json'}, signal:controller.signal}); }
        finally { clearTimeout(timer); }
        if (response.status === 404) {
          setText('#fwReleaseStatus', 'Aucune version stable publiée pour le moment. Le firmware main reste disponible après le premier build.');
          return;
        }
        if (!response.ok) throw Error(`GitHub API : HTTP ${response.status}`);
        const release = await response.json();
        const asset = latestReleaseAsset(release, repo);
        if (!asset) {
          setText('#fwReleaseStatus', 'La dernière release ne contient pas de fichier UF2 RP2040. Consultez les releases GitHub.');
          return;
        }
        showLink(stableButton, asset.url, '↓ Télécharger le firmware stable');
        setText('#fwReleaseStatus', `${asset.tag} · ${prettyDate(release.published_at)}${asset.size ? ' · '+(asset.size/1024).toFixed(0)+' Kio' : ''}`);
      } catch (error) {
        setText('#fwReleaseStatus', 'Impossible de consulter les releases (connexion, API GitHub ou dépôt privé). ' + error.message);
      }
    } finally {
      refreshing = false;
      refresh.disabled = false;
    }
  }
  refresh.addEventListener('click', () => void update());
  void update();
}
