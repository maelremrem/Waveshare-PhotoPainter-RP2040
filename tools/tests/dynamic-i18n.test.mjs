import test from 'node:test';
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';

test('dynamic UI modules translate labels and initialize before rendering', async () => {
  const app = await readFile(new URL('../js/app.js', import.meta.url), 'utf8');
  assert.match(app, /await initI18n\(\)/);
  assert.match(app, /t\(PALETTE_LABELS\[i\]\)/);
  assert.match(app, /t\(ALGORITHMS\[name\]\)/);
  for (const name of ['app', 'frame-ui', 'firmware-ui']) {
    const source = await readFile(new URL(`../js/${name}.js`, import.meta.url), 'utf8');
    assert.match(source, /languagechange/);
  }
  const worker = await readFile(new URL('../js/worker.js', import.meta.url), 'utf8');
  assert.doesNotMatch(worker, /i18n\.js/);
});
