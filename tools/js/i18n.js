// Flat dictionaries use the original French message as their stable key.
const STORAGE_KEY = 'photopainter-language';
const dictionaries = { fr: {}, en: {} };
let language = 'fr';
let initialization;
const bindings = new WeakMap();
const attributes = ['title', 'aria-label', 'placeholder', 'alt', 'label', 'content'];
const own = (object, key) => Object.prototype.hasOwnProperty.call(object, key);

export function getLanguage() { return language; }

export function t(key, params = {}) {
  const message = own(dictionaries[language], key) ? dictionaries[language][key]
    : own(dictionaries.fr, key) ? dictionaries.fr[key] : key;
  return String(message).replace(/\{([^{}]+)\}/g, (token, name) =>
    own(params, name) ? String(params[name]) : token);
}

function applyTranslations() {
  document.documentElement.lang = language;
  document.querySelectorAll('[data-i18n-text]').forEach(element => {
    let entries = bindings.get(element);
    if (!entries) {
      entries = Object.entries(JSON.parse(element.getAttribute('data-i18n-text')))
        .map(([index, key]) => {
          const node = element.childNodes[Number(index)];
          if (!node || node.nodeType !== 3) return null;
          const match = node.nodeValue.match(/^(\s*)[\s\S]*?(\s*)$/);
          return { node, key, before: match[1], after: match[2], last: node.nodeValue };
        }).filter(Boolean);
      bindings.set(element, entries);
    }
    entries.forEach(entry => {
      // A dynamic UI owns nodes it has replaced or rewritten since translation.
      // Its languagechange listener must re-render those using t().
      if (entry.node.parentNode !== element || entry.node.nodeValue !== entry.last) return;
      entry.node.nodeValue = entry.before + t(entry.key) + entry.after;
      entry.last = entry.node.nodeValue;
    });
  });
  attributes.forEach(attribute => {
    document.querySelectorAll(`[data-i18n-${attribute}]`).forEach(element => {
      element.setAttribute(attribute, t(element.getAttribute(`data-i18n-${attribute}`)));
    });
  });
  const select = document.getElementById('languageSelect');
  if (select) select.value = language;
}

export function setLanguage(value) {
  language = value === 'en' ? 'en' : 'fr';
  try { localStorage.setItem(STORAGE_KEY, language); } catch { /* Private browsing / denied storage. */ }
  applyTranslations();
  document.dispatchEvent(new CustomEvent('languagechange', { detail: { language } }));
}

export async function initI18n() {
  if (initialization) return initialization;
  initialization = (async () => {
    await Promise.all(['fr', 'en'].map(async lang => {
      try {
        const response = await fetch(new URL(`../locales/${lang}.json`, import.meta.url));
        if (!response.ok) throw new Error(`Dictionary HTTP ${response.status}`);
        const data = await response.json();
        if (!data || Array.isArray(data) || typeof data !== 'object') throw new Error('Invalid dictionary');
        dictionaries[lang] = Object.fromEntries(Object.entries(data).filter(([, value]) => typeof value === 'string'));
      } catch (error) { console.warn(`Could not load ${lang} translations; using French source text.`, error); }
    }));
    let stored;
    try { stored = localStorage.getItem(STORAGE_KEY); } catch { /* Storage is optional. */ }
    language = stored === 'fr' || stored === 'en' ? stored
      : /^en(?:-|$)/i.test(globalThis.navigator?.language || '') ? 'en' : 'fr';
    applyTranslations();
    document.getElementById('languageSelect')?.addEventListener('change', event => setLanguage(event.target.value));
    return language;
  })();
  return initialization;
}
