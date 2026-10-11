const VERSION='photopainter-v0.4.1';
const ASSETS=['./','./index.html','./assets/style.css','./assets/favicon.svg','./manifest.webmanifest','./js/app.js','./js/engine.js','./js/worker.js','./js/zip.js','./js/epd.js','./js/frame-config.js','./js/frame-ui.js','./js/firmware-ui.js'];
self.addEventListener('install',e=>{e.waitUntil(caches.open(VERSION).then(c=>c.addAll(ASSETS)).then(()=>self.skipWaiting()));});
self.addEventListener('activate',e=>{e.waitUntil(caches.keys().then(keys=>Promise.all(keys.filter(k=>k!==VERSION).map(k=>caches.delete(k)))).then(()=>self.clients.claim()));});
self.addEventListener('fetch',e=>{if(e.request.method!=='GET'||new URL(e.request.url).origin!==self.location.origin)return;
  if(new URL(e.request.url).pathname.endsWith('.uf2') || new URL(e.request.url).pathname.endsWith('/firmware/build.json'))return;
  e.respondWith(fetch(e.request).then(r=>{if(r.ok){const clone=r.clone();caches.open(VERSION).then(c=>c.put(e.request,clone));}return r;}).catch(()=>caches.match(e.request)));});
