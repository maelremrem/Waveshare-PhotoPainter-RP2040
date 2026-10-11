import test from 'node:test';
import assert from 'node:assert/strict';
import {startFirmwareUI} from '../js/firmware-ui.js';

test('firmware language changes rerender cached results without fetching or adding handlers', async () => {
  const elements = new Map();
  const listeners = new Map();
  const previous = {document:globalThis.document, location:globalThis.location, fetch:globalThis.fetch};
  let fetches=0;
  globalThis.document = {
    querySelector(selector) {
      if (!elements.has(selector)) elements.set(selector,{hidden:false,textContent:'',addEventListener(name,handler){this.handlers??=[];this.handlers.push(handler);}});
      return elements.get(selector);
    },
    addEventListener(name, handler) { listeners.set(name,handler); }
  };
  globalThis.location = {href:'https://example.github.io/project/'};
  globalThis.fetch = async () => {fetches++;return {ok:true,status:200,json:async()=>({repository:'example/project',version:'1.0',file:'PhotoPainter-RP2040-main.uf2',bytes:512,sha256:'a'.repeat(64),built_at:'2026-01-01T12:00:00Z'})};};
  try {
    startFirmwareUI();
    await new Promise(resolve=>setTimeout(resolve,0));
    const count=fetches;
    const url=elements.get('#fwDevDownload').href;
    listeners.get('languagechange')();
    listeners.get('languagechange')();
    assert.equal(fetches,count);
    assert.equal(elements.get('#fwDevDownload').href,url);
    assert.equal(elements.get('#fwDevDownload').hidden,false);
    assert.equal(elements.get('#fwRefresh').handlers.length,1);
    assert.match(elements.get('#fwBuildStatus').textContent,/1\.0/);
  } finally {
    for (const [key,value] of Object.entries(previous)) value===undefined?delete globalThis[key]:globalThis[key]=value;
  }
});
