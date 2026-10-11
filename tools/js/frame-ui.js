// Local-only settings.conf creator + optional Web Serial command console.
import {FRAME_DEFAULTS,validateFrame,serializeFrame,parseFrame} from './frame-config.js';
import {saveFile} from './zip.js';
const $=s=>document.querySelector(s);
const $$=s=>[...document.querySelectorAll(s)];
const STORAGE_KEY='photopainterFrameSettingsV1';
let current={...FRAME_DEFAULTS};
try{const x=JSON.parse(localStorage.getItem(STORAGE_KEY)||'null');if(x){const v=validateFrame(x);if(!v.errors.length)current=v.settings;}}catch{}
function sync(){for(const el of $$('[data-frame-setting]')){const key=el.dataset.frameSetting;el.type==='checkbox'?el.checked=!!current[key]:el.value=String(current[key]);}update();}
function read(){const data={};for(const el of $$('[data-frame-setting]'))data[el.dataset.frameSetting]=el.type==='checkbox'?el.checked:el.value;
  const {settings,errors}=validateFrame(data);return {settings,errors};}
function update(){const {settings,errors}=read();
  const output=$('#frameConfText');
  try{output.value=serializeFrame(settings);}catch{output.value='# Corriger les erreurs ci-dessous avant export';}
  $('#frameValidation').textContent=errors.length?errors.join(' · '):'Configuration valide · prête pour la racine de la microSD';
  $('#frameValidation').classList.toggle('invalid',!!errors.length);
  $('#frameDownload').disabled=!!errors.length;
  if(!errors.length){current=settings;try{localStorage.setItem(STORAGE_KEY,JSON.stringify(current));}catch{}}
}
export function getFrameConf(){return serializeFrame(current);}
export function startFrameUI(){
  for(const el of $$('[data-frame-setting]'))el.addEventListener('input',update);
  $('#frameDownload').addEventListener('click',()=>{try{saveFile(new TextEncoder().encode(getFrameConf()),'settings.conf','text/plain');}catch(e){$('#frameValidation').textContent=e.message;}});
  $('#frameImport').addEventListener('change',async ev=>{
    const file=ev.target.files?.[0];if(!file)return;
    const parsed=parseFrame(await file.text());
    if(parsed.errors.length){$('#frameValidation').textContent=parsed.errors.join(' · ');ev.target.value='';return;}
    current=parsed.settings;sync();ev.target.value='';
  });
  $('#frameReset').onclick=()=>{current={...FRAME_DEFAULTS};sync();};
  sync();
  initSerial();
}
function initSerial(){
  const connect=$('#serialConnect'),send=$('#serialSend'),cmd=$('#serialCommand'),log=$('#serialLog');
  let port=null,reader=null,writer=null;const enc=new TextEncoder();
  const append=s=>{log.textContent+=s;log.scrollTop=log.scrollHeight;};
  if(!('serial' in navigator)){connect.disabled=true;append('Web Serial non disponible. Utilise Chromium/Chrome en HTTPS ou localhost.\n');return;}
  const sendText=async s=>{if(!writer){append('[USB] Connectez d’abord le cadre.\n');return;}await writer.write(enc.encode(s.trim()+'\n'));};
  connect.onclick=async()=>{
    if(port){
      const previousPort=port;port=null;
      try{await reader?.cancel();}catch{}
      try{writer?.releaseLock();}catch{}writer=null;
      try{await previousPort.close();}catch(e){append('[USB] '+e.message+'\n');}
      connect.textContent='Connecter le RP2040';send.disabled=true;
      append('[USB] Déconnecté\n');return;
    }
    try{
      port=await navigator.serial.requestPort();await port.open({baudRate:115200});
      writer=port.writable.getWriter();connect.textContent='Déconnecter USB';send.disabled=false;
      reader=port.readable.getReader();append('[USB] Connecté à 115200 bauds\n');
      (async()=>{try{const decoder=new TextDecoder();while(true){const {value,done}=await reader.read();if(done)break;if(value)append(decoder.decode(value,{stream:true}));}}catch(e){append('\n[USB] '+e.message+'\n');}finally{try{reader?.releaseLock();}catch{}reader=null;}})();
    }catch(e){append('[USB] '+e.message+'\n');if(port){try{await port.close();}catch{}}port=null;connect.textContent='Connecter le RP2040';send.disabled=true;}
  };
  send.onclick=async()=>{try{if(cmd.value.trim())await sendText(cmd.value);}catch(e){append(e.message+'\n');}};
  $('#serialStatus').onclick=()=>sendText('STATUS').catch(e=>append(e.message+'\n'));
  $('#serialNext').onclick=()=>sendText('NEXT').catch(e=>append(e.message+'\n'));
  $('#serialSyncTime').onclick=()=>{
    const d=new Date(),two=n=>String(n).padStart(2,'0');
    sendText(`SETTIME ${d.getFullYear()}-${two(d.getMonth()+1)}-${two(d.getDate())} ${two(d.getHours())}:${two(d.getMinutes())}:${two(d.getSeconds())}`)
       .catch(e=>append(e.message+'\n'));
  };
  cmd.addEventListener('keydown',e=>{if(e.key==='Enter')send.click();});
}
