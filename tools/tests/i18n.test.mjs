import test from 'node:test';
import assert from 'node:assert/strict';

test('i18n loads English, interpolates safely, and falls back to French source', async () => {
  let api;
  try { api = await import('../js/i18n.js'); } catch {}
  assert.ok(api, 'i18n runtime exists');
  Object.defineProperty(globalThis, 'navigator', { configurable: true, value: { language: 'en-US' } });
  globalThis.localStorage = { getItem: () => null, setItem() {} };
  globalThis.document = { documentElement: {}, querySelectorAll: () => [], getElementById: () => null, dispatchEvent() {} };
  globalThis.fetch = async url => ({ ok: true, json: async () => JSON.parse(await (await import('node:fs/promises')).readFile(url, 'utf8')) });
  await api.initI18n();
  assert.equal(api.getLanguage(), 'en');
  assert.equal(document.documentElement.lang, 'en');
  assert.equal(api.t('Réinitialiser'), 'Reset');
  assert.equal(api.t('Absent {name}', { name: '<img>' }), 'Absent <img>');
  assert.equal(api.t('Unknown'), 'Unknown');
});

test('language switching preserves mixed children, persists choice, and emits event', async () => {
  const api = await import('../js/i18n.js?mixed');
  const child = { nodeType: 1, listener: () => 'still attached' };
  const text = { nodeType: 3, nodeValue: ' Réinitialiser ' };
  const element = { childNodes: [child, text], getAttribute: () => '{"1":"Réinitialiser"}' };
  text.parentNode = element;
  const events = [];
  const saved = [];
  globalThis.CustomEvent = class { constructor(type, options) { this.type = type; this.detail = options.detail; } };
  globalThis.localStorage = { getItem: () => 'fr', setItem: (...args) => saved.push(args) };
  globalThis.document = { documentElement: {}, getElementById: () => null,
    querySelectorAll: selector => selector === '[data-i18n-text]' ? [element] : [],
    dispatchEvent: event => events.push(event) };
  await api.initI18n();
  assert.equal(api.getLanguage(), 'fr');
  api.setLanguage('en');
  assert.equal(text.nodeValue, ' Reset ');
  assert.equal(element.childNodes[0], child);
  assert.equal(child.listener(), 'still attached');
  assert.equal(events[0].type, 'languagechange');
  assert.deepEqual(events[0].detail, { language: 'en' });
  assert.deepEqual(saved[0], ['photopainter-language', 'en']);
  api.setLanguage('fr');
  assert.equal(text.nodeValue, ' Réinitialiser ');
  text.nodeValue = 'Dynamic content';
  api.setLanguage('en');
  assert.equal(text.nodeValue, 'Dynamic content', 'static translation must not overwrite dynamic UI');
});

test('unsupported browser language falls back to French when storage is unavailable', async () => {
  const api = await import('../js/i18n.js?fallback');
  Object.defineProperty(globalThis, 'navigator', { configurable: true, value: { language: 'de-DE' } });
  globalThis.localStorage = { getItem() { throw new Error('blocked'); }, setItem() { throw new Error('blocked'); } };
  globalThis.document = { documentElement: {}, querySelectorAll: () => [], getElementById: () => null, dispatchEvent() {} };
  await api.initI18n();
  assert.equal(api.getLanguage(), 'fr');
  api.setLanguage('en');
  assert.equal(api.t('Réinitialiser'), 'Reset');
});

test('static marker keys all exist in both flat dictionaries', async () => {
  const { readFile } = await import('node:fs/promises');
  const fr = JSON.parse(await readFile(new URL('../locales/fr.json', import.meta.url), 'utf8'));
  const en = JSON.parse(await readFile(new URL('../locales/en.json', import.meta.url), 'utf8'));
  assert.deepEqual(Object.keys(fr).sort(), Object.keys(en).sort());
  for (const [key, value] of Object.entries(fr)) {
    assert.equal(value, key);
    assert.equal(typeof en[key], 'string');
    assert.ok(en[key].length > 0);
  }
  const html = await readFile(new URL('../index.html', import.meta.url), 'utf8');
  assert.match(html, /id="languageSelect"/);
  assert.ok(html.indexOf('id="languageSelect"') < html.indexOf('class="top-actions"'));
});
