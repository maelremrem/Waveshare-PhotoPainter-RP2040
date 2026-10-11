import test from 'node:test';
import assert from 'node:assert/strict';
import {repositoryFromSite, verifiedBuild, latestReleaseAsset} from '../js/firmware-ui.js';

test('detects the GitHub Pages repository on project and user sites', () => {
  assert.equal(repositoryFromSite('https://maelremrem.github.io/PhotoPainter-RP2040/'), 'maelremrem/PhotoPainter-RP2040');
  assert.equal(repositoryFromSite('https://example.github.io/'), 'example/example.github.io');
  assert.equal(repositoryFromSite('http://localhost:8000/'), '');
  assert.equal(repositoryFromSite('https://github.com/foo/bar'), '');
});

test('accepts only validated build metadata', () => {
  const sample = {repository:'owner/PhotoPainter', version:'0.4.1', file:'PhotoPainter-RP2040-main.uf2', bytes:4096, sha256:'a'.repeat(64)};
  assert.deepEqual(verifiedBuild(sample), sample);
  assert.equal(verifiedBuild({...sample, file:'../evil.uf2'}), null);
  assert.equal(verifiedBuild({...sample, repository:'evil.repo/../'}), null);
  assert.equal(verifiedBuild({...sample, sha256:'invalid'}), null);
  assert.equal(verifiedBuild({...sample, bytes:77}), null);
});

test('only accepts the exact UF2 asset from the correct GitHub repository', () => {
  const good = {tag_name:'v0.4.1', assets:[{name:'PhotoPainter-RP2040.uf2', size:5120, browser_download_url:'https://github.com/owner/photo/releases/download/v0.4.1/PhotoPainter-RP2040.uf2'}]};
  assert.deepEqual(latestReleaseAsset(good, 'owner/photo'), {tag:'v0.4.1', url:good.assets[0].browser_download_url, size:5120});
  assert.equal(latestReleaseAsset(good,'another/repo'), null);
  assert.equal(latestReleaseAsset({...good,assets:[{...good.assets[0],browser_download_url:'https://evil.example/evil.uf2'}]}, 'owner/photo'), null);
  assert.equal(latestReleaseAsset({...good,assets:[]}, 'owner/photo'), null);
});
