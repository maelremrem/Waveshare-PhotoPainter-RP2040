import {DEFAULTS, PRESETS, PALETTE_LABELS, ALGORITHMS, MONO_MODES, paletteFor, bmp24, outputSize, sanitizeBaseName} from './engine.js';
import {makeZip, saveFile} from './zip.js';
import {encodeEPD} from './epd.js';
import {startFrameUI, getFrameConf} from './frame-ui.js';
import {startFirmwareUI} from './firmware-ui.js';
const $=s=>document.querySelector(s);
const $$=s=>[...document.querySelectorAll(s)];
const worker=new Worker(new URL('./worker.js',import.meta.url),{type:'module'});
const pending=new Map();let nextRequest=0;
worker.addEventListener('message',({data})=>{
  const entry=pending.get(data.id);if(!entry)return;pending.delete(data.id);
  if(data.type==='error')entry.reject(Error(data.message));
  else entry.resolve({width:data.width,height:data.height,data:new Uint8ClampedArray(data.pixels)});
});
worker.addEventListener('error',e=>{for(const p of pending.values())p.reject(Error(e.message||'Erreur de traitement'));pending.clear();});
function processFile(file,options){return new Promise((resolve,reject)=>{
  const id=++nextRequest;pending.set(id,{resolve,reject});worker.postMessage({id,file,config:{...options}});
});}
const state={files:[],selected:null,settings:{...DEFAULTS},preset:'natural',preview:null,revision:0,exporting:false,comparing:false};
let renderTimeout=null;
const allowed=/\.(jpe?g|png|webp|bmp)$/i;
const activeSettings=()=>state.selected?.personalSettings||state.settings;
const formatSize=n=>n>1048576?(n/1048576).toFixed(1)+' Mo':Math.max(1,Math.round(n/1024))+' Ko';
function message(text){$('#statusMessage').textContent=text;}
function storageSave(){try{localStorage.setItem('photopainterStudioSettingsV2',JSON.stringify({settings:state.settings,preset:state.preset}));}catch{}}
function storageLoad(){try{const raw=JSON.parse(localStorage.getItem('photopainterStudioSettingsV2')||'null');if(raw?.settings){state.settings=validateSettings(raw.settings);state.preset=typeof raw.preset==='string'?raw.preset:'custom';}}catch{}}
function validateSettings(incoming){const safe={...DEFAULTS};
  for(const [key,defaultValue] of Object.entries(DEFAULTS)){
    const v=incoming[key];
    if(typeof defaultValue==='number' && Number.isFinite(Number(v)) && v!==undefined && v!==null)safe[key]=Number(v);
    else if(typeof defaultValue==='boolean' && typeof v==='boolean')safe[key]=v;
    else if(typeof defaultValue==='string' && typeof v==='string')safe[key]=v;
  }
  if(!['none','floyd','atkinson','sierra-lite','burkes','stucki','jjn','bayer4','bayer8','bayer16','noise'].includes(safe.algorithm))safe.algorithm='floyd';
  if(!['oklab','weighted','rgb'].includes(safe.distance))safe.distance='oklab';
  if(!Object.hasOwn(MONO_MODES,safe.monoMode))safe.monoMode='off';
  safe.zoom=Math.max(1,Math.min(3,safe.zoom));
  safe.panX=Math.max(-100,Math.min(100,safe.panX));
  safe.panY=Math.max(-100,Math.min(100,safe.panY));
  safe.monoBlackPoint=Math.max(0,Math.min(250,safe.monoBlackPoint));
  safe.monoWhitePoint=Math.max(5,Math.min(255,safe.monoWhitePoint));
  safe.monoThreshold=Math.max(0,Math.min(255,safe.monoThreshold));
  if(!['contain','cover'].includes(safe.fit))safe.fit='contain';
  if(![0,90,180,270].includes(safe.orientation))safe.orientation=0;
  if(!/^#[0-9a-f]{6}$/i.test(safe.background))safe.background='#ffffff';
  return safe;
}
function syncControls(){
  for(const input of $$('[data-setting]')){
    const key=input.dataset.setting;if(key==='preset')continue;
    const value=activeSettings()[key];
    if(input.type==='checkbox')input.checked=!!value;
    else input.value=String(value);
    if(input.type==='range')updateRangeLabel(input);
  }
  $('#presetSelect').value=PRESETS[state.preset]?state.preset:'custom';
  updateModeControls();
  $('#perImageSettings').checked=!!state.selected?.personalSettings;
  $('#perImageSettings').disabled=!state.selected;
  const dims=outputSize(activeSettings().orientation);
  $('#resultDimensions').textContent=`${dims.width} × ${dims.height}`;
}
function updateModeControls(){
  const config=activeSettings();
  const mono=config.monoMode!=='off';
  const threshold=config.monoMode==='threshold';
  $('#monoControls').hidden=!mono;
  $('#monoThresholdControl').hidden=!threshold;
  $('#monoHint').hidden=!mono;
  $('#monoHint').textContent=threshold
    ? 'Image binaire sans tramage : réglez le seuil pour les dessins et les logos.'
    : 'Les gris sont simulés avec des pixels noirs et blancs ; aucun gris artificiel dans le BMP.';
  const ordered=['bayer4','bayer8','bayer16','noise'].includes(config.algorithm);
  $('[data-range="grainPenalty"]').hidden=!ordered||threshold;
  $('[data-setting="serpentine"]').disabled=threshold||['none','bayer4','bayer8','bayer16','noise'].includes(config.algorithm);
  $('#algorithmSelect').disabled=threshold;
  $('#strength').disabled=threshold;
  $('#compareBtn').disabled=threshold||!state.selected||state.exporting||state.comparing;
  $('#resultType').textContent=mono?'RÉSULTAT NOIR & BLANC':'RÉSULTAT 7 COULEURS';
  createPalette();
}
function updateRangeLabel(input){
  const n=Number(input.value);let suffix='';
  if(['strength','grainPenalty','denoise'].includes(input.id))suffix=Math.round(n*100)+' %';
  else if(['gamma','contrast','saturation','sharpen','zoom'].includes(input.id))suffix=n.toFixed(2)+'×';
  else if(input.id==='exposure')suffix=(n>0?'+':'')+n.toFixed(1)+' EV';
  else suffix=(n>0?'+':'')+n;
  const out=document.getElementById(input.id+'Value');if(out)out.textContent=suffix;
}
function createPalette(){const root=$('#paletteLegend');root.replaceChildren();
  const legend=document.createElement('span');legend.className='legend-label';legend.textContent=activeSettings().monoMode==='off'?'PALETTE MATÉRIELLE':'PALETTE N&B';root.append(legend);
  paletteFor(activeSettings()).forEach((rgb,i)=>{
  const el=document.createElement('span');el.className='swatch';el.title=PALETTE_LABELS[i];
  el.style.backgroundColor=`rgb(${rgb.join(',')})`;root.append(el);
});}
function setActionState(){const active=!!state.selected;$('#fileCount').textContent=String(state.files.length);
  $('#exportSelected').disabled=!active||state.exporting;$('#exportAll').disabled=!state.files.length||state.exporting;
  $('#clearLibrary').disabled=!state.files.length||state.exporting;
  $('#compareBtn').disabled=!active||state.exporting||state.comparing||activeSettings().monoMode==='threshold';
  $('#savePreview').disabled=!state.preview||state.exporting;
}
function renderFileList(){const root=$('#fileList');root.replaceChildren();
  if(!state.files.length){const e=document.createElement('div');e.className='empty-list';e.textContent='Vos images apparaîtront ici.';root.append(e);}
  state.files.forEach((record,index)=>{
    const btn=document.createElement('button');btn.className='file-item'+(record.id===state.selected?.id?' active':'');
    btn.setAttribute('aria-label','Sélectionner '+record.file.name);
    const thumb=document.createElement('img');thumb.className='file-thumb';thumb.src=record.url;thumb.alt='';thumb.loading='lazy';
    const meta=document.createElement('span');meta.className='file-meta';
    const title=document.createElement('span');title.className='file-title';title.title=record.file.name;title.textContent=record.file.name;
    const size=document.createElement('span');size.className='file-size';size.textContent=formatSize(record.file.size);
    meta.append(title,size);
    const id=document.createElement('span');id.className='file-index';id.textContent=String(index+1).padStart(2,'0');
    btn.append(thumb,meta,id);btn.addEventListener('click',()=>selectFile(record));root.append(btn);
  });setActionState();
}
let idSeq=0;
function importFiles(list){
  const files=[...list].filter(file=>allowed.test(file.name));
  if(!files.length){message('Aucune image JPG, PNG, WEBP ou BMP valide dans la sélection.');return;}
  for(const file of files){if(state.files.length>=150){message('Limite de 150 photos atteinte.');break;}
    const id=++idSeq,url=URL.createObjectURL(file);state.files.push({id,file,url});
  }
  renderFileList();if(!state.selected)selectFile(state.files[0]);
  else message(`${files.length} photo(s) ajoutée(s) – ${state.files.length} au total`);
}
function selectFile(record){
  state.selected=record;state.preview=null;state.revision++;
  $('#selectedFilename').textContent=record.file.name;
  $('#originalImage').src=record.url;$('#originalImage').hidden=false;
  $('#originalEmpty').hidden=true;$('#resultEmpty').hidden=false;
  $('#originalImage').onload=()=>$('#sourceDimensions').textContent=`${$('#originalImage').naturalWidth} × ${$('#originalImage').naturalHeight}`;
  syncControls();renderFileList();scheduleRender(0);
}
function scheduleRender(wait=420){clearTimeout(renderTimeout);renderTimeout=setTimeout(()=>{void updatePreview();},wait);}
async function updatePreview(){
  const record=state.selected;if(!record)return;
  const revision=++state.revision,settings={...activeSettings()};
  $('#previewLoading').hidden=false;$('#resultEmpty').hidden=true;
  message('Préparation de l’aperçu…');
  try {
    const image=await processFile(record.file,settings);
    if(revision!==state.revision||state.selected?.id!==record.id)return;
    const canvas=$('#resultCanvas');canvas.width=image.width;canvas.height=image.height;
    canvas.getContext('2d').putImageData(new ImageData(image.data,image.width,image.height),0,0);
    state.preview={fileId:record.id,image,key:JSON.stringify(settings)};
    $('#resultDimensions').textContent=`${image.width} × ${image.height}`;
    $('#resultEmpty').hidden=true;
    message(`${record.file.name} · ${MONO_MODES[settings.monoMode]||'7 couleurs'} · ${settings.monoMode==='threshold'?'sans tramage':ALGORITHMS[settings.algorithm]} · prêt`);
  }catch(err){if(revision===state.revision){$('#resultEmpty').hidden=false;$('#resultEmpty').textContent='Impossible de décoder cette image';message('Erreur : '+err.message);}}
  finally{if(revision===state.revision){$('#previewLoading').hidden=true;setActionState();}}
}
function fileBaseName(record,used,extension){let base=sanitizeBaseName(record.file.name),name=base,n=2;
  while(used.has(name.toLowerCase()))name=`${base}_${n++}`;
  used.add(name.toLowerCase());return name+'.'+extension;
}
function exportFormat(){return $('#exportFormat').value;}
function imageBytes(image,settings){return exportFormat()==='epd'?encodeEPD(image,settings.orientation):bmp24(image);}
function downloadImage(record,image,settings){const extension=exportFormat(),bytes=imageBytes(image,settings);
  saveFile(bytes,sanitizeBaseName(record.file.name)+'.'+extension,extension==='epd'?'application/octet-stream':'image/bmp');}
function updateExportLabels(){const ext=exportFormat().toUpperCase();$('#exportSelected').innerHTML=`Exporter le ${ext} <span aria-hidden="true">↗</span>`;
  $('#exportFormatHint').textContent=ext==='EPD'?'192 024 octets/image · aucun décodage sur le RP2040':'BMP 24 bits · compatibilité anciennes versions';
}
async function ensurePreview(record,config){const key=JSON.stringify(config);if(state.preview?.fileId===record.id&&state.preview.key===key)return state.preview.image;return processFile(record.file,config);}
async function exportSelected(){const record=state.selected;if(!record||state.exporting)return;
  state.exporting=true;setActionState();message('Préparation de l’export…');
  try{const settings={...activeSettings()};const image=await ensurePreview(record,settings);downloadImage(record,image,settings);message('Image téléchargée : '+sanitizeBaseName(record.file.name)+'.'+exportFormat());}
  catch(err){message('Export impossible : '+err.message);}finally{state.exporting=false;setActionState();}
}
async function exportAll(){if(!state.files.length||state.exporting)return;
  state.exporting=true;setActionState();$('#progressPanel').hidden=false;
  const files=[...state.files],options={...state.settings},names=new Set(),results=[],ext=exportFormat();let failed=0;
  try{
    for(let i=0;i<files.length;i++){
      const record=files[i];$('#progressTitle').textContent='Préparation des images…';
      $('#progressText').textContent=`${i+1} / ${files.length}`;
      $('#exportProgress').value=100*i/files.length;
      message(`Conversion ${i+1}/${files.length} — ${record.file.name}`);
      await new Promise(r=>setTimeout(r,0));
      try{const config=record.personalSettings||options;const image=await ensurePreview(record,config);
        results.push({name:'pic/'+fileBaseName(record,names,ext),data:ext==='epd'?encodeEPD(image,config.orientation):bmp24(image)});
      }catch(e){failed++;console.warn('Conversion échouée :',record.file.name,e);}
    }
    if(!results.length)throw Error('Aucune image convertible');
    if($('#includeFrameConf').checked){results.push({name:'settings.conf',data:new TextEncoder().encode(getFrameConf())});}
    $('#progressTitle').textContent='Création de l’archive ZIP…';
    const zip=makeZip(results);saveFile(zip,'PhotoPainter_SD_pic.zip','application/zip');
    $('#exportProgress').value=100;message(`${files.length-failed} ${ext.toUpperCase()} exportés dans un ZIP${failed?' · '+failed+' erreur(s)':''}`);
  }catch(err){message('Export impossible : '+err.message);}
  finally{state.exporting=false;setActionState();setTimeout(()=>{$('#progressPanel').hidden=true;},1500);}
}
function changeSettings(event){const input=event.currentTarget,key=input.dataset.setting;
  if(!key||key==='preset')return;
  let value=input.type==='checkbox'?input.checked:input.type==='range'?Number(input.value):input.value;
  if(key==='orientation')value=Number(value);
  activeSettings()[key]=value;state.preset='custom';$('#presetSelect').value='custom';
  if(input.type==='range')updateRangeLabel(input);
  updateModeControls();
  const {width,height}=outputSize(activeSettings().orientation);
  $('#resultDimensions').textContent=`${width} × ${height}`;
  state.preview=null;setActionState();storageSave();scheduleRender();
}
async function exportPreview(){const canvas=$('#resultCanvas');if(!state.preview)return;
  const png=await new Promise(resolve=>canvas.toBlob(resolve,'image/png'));
  if(png)saveFile(png,sanitizeBaseName(state.selected.file.name)+'_preview.png');
}
async function compare(){if(!state.selected||state.comparing)return;
  const dialog=$('#compareDialog'),grid=$('#compareGrid');grid.replaceChildren();
  const selected=state.selected,base={...activeSettings()};const options=['none','floyd','atkinson','bayer8','stucki','jjn'];
  state.comparing=true;setActionState();dialog.showModal();
  try{
    for(const name of options){if(!dialog.open)break;
      const item=document.createElement('button');item.className='compare-option busy';item.type='button';
      const canvas=document.createElement('canvas'),title=document.createElement('strong'),subtitle=document.createElement('span');
      title.textContent=ALGORITHMS[name];subtitle.textContent='Traitement…';item.append(canvas,title,subtitle);grid.append(item);
      try{
        const result=await processFile(selected.file,{...base,algorithm:name});
        canvas.width=result.width;canvas.height=result.height;
        canvas.getContext('2d').putImageData(new ImageData(result.data,result.width,result.height),0,0);
        subtitle.textContent='Cliquer pour appliquer';item.classList.remove('busy');
        if(name===activeSettings().algorithm)item.classList.add('selected');
        item.onclick=()=>{activeSettings().algorithm=name;state.preset='custom';syncControls();storageSave();state.preview=null;scheduleRender(0);dialog.close();};
      }catch(err){subtitle.textContent=err.message;item.classList.remove('busy');item.disabled=true;}
    }
  }finally{state.comparing=false;setActionState();}
}
function downloadSettings(){const json=JSON.stringify({app:'PhotoPainter Studio Web',version:1,settings:state.settings},null,2);saveFile(new Blob([json],{type:'application/json'}),'photopainter-presets.json','application/json');}
function clearLibrary(){if(state.exporting)return;for(const record of state.files)URL.revokeObjectURL(record.url);
  state.files=[];state.selected=null;state.preview=null;state.revision++;
  $('#originalImage').hidden=true;$('#originalImage').removeAttribute('src');
  $('#originalEmpty').hidden=false;$('#resultEmpty').hidden=false;$('#sourceDimensions').textContent='—';
  $('#selectedFilename').textContent='Importez une image pour commencer';
  $('#resultCanvas').getContext('2d').clearRect(0,0,800,800);
  renderFileList();message('Bibliothèque vidée');
}
function wireEvents(){
  $('#exportFormat').onchange=()=>{updateExportLabels();};
  $('#tabStudio').onclick=()=>switchPage('studio');
  $('#tabFrame').onclick=()=>switchPage('frame');
  $('#tabFirmware').onclick=()=>switchPage('firmware');
  $('#addImages').onclick=()=>$('#fileInput').click();
  $('#dropzone').onclick=()=>$('#fileInput').click();
  $('#dropzone').addEventListener('keydown',e=>{if(e.key==='Enter'||e.key===' '){e.preventDefault();$('#fileInput').click();}});
  $('#fileInput').addEventListener('change',e=>{importFiles(e.target.files);e.target.value='';});
  for(const target of [$('#dropzone'),$('#stageShell')]){
    target.addEventListener('dragover',e=>{e.preventDefault();$('#dropzone').classList.add('dragging');});
    target.addEventListener('dragleave',()=>$('#dropzone').classList.remove('dragging'));
    target.addEventListener('drop',e=>{e.preventDefault();$('#dropzone').classList.remove('dragging');importFiles(e.dataTransfer.files);});
  }
  $('#perImageSettings').onchange=e=>{if(!state.selected)return;
    state.selected.personalSettings=e.target.checked?{...activeSettings()}:null;
    syncControls();storageSave();state.preview=null;scheduleRender(0);
  };
  for(const input of $$('[data-setting]'))if(input.id!=='presetSelect')input.addEventListener(input.type==='range'?'input':'change',changeSettings);
  $('#presetSelect').addEventListener('change',e=>{
    const preset=e.target.value;
    if(!PRESETS[preset])return;
    Object.assign(activeSettings(),PRESETS[preset]);state.preset=preset;
    syncControls();state.preview=null;storageSave();scheduleRender(0);
  });
  $('#settingsReset').onclick=()=>{if(state.selected?.personalSettings)state.selected.personalSettings={...DEFAULTS};else state.settings={...DEFAULTS};state.preset='natural';syncControls();storageSave();state.preview=null;scheduleRender(0);message('Réglages réinitialisés');};
  $('#exportSelected').onclick=()=>void exportSelected();
  $('#exportAll').onclick=()=>void exportAll();
  $('#savePreview').onclick=()=>void exportPreview();
  let drag=null;
  $('#resultCanvas').addEventListener('pointerdown',e=>{
    if(!state.selected)return;drag={x:e.clientX,y:e.clientY};e.currentTarget.setPointerCapture(e.pointerId);
  });
  $('#resultCanvas').addEventListener('pointerup',e=>{
    if(!drag||!state.selected)return;
    const canvas=e.currentTarget,rect=canvas.getBoundingClientRect();
    const s=activeSettings(),zoom=Number(s.zoom)||1;
    // Even at 1x, cover mode may be cropped; sliders give exact control.
    const factor=zoom>1||s.fit==='cover'?1:0;
    s.panX=Math.max(-100,Math.min(100,(Number(s.panX)||0)-(e.clientX-drag.x)*200/Math.max(1,rect.width)*factor));
    s.panY=Math.max(-100,Math.min(100,(Number(s.panY)||0)-(e.clientY-drag.y)*200/Math.max(1,rect.height)*factor));
    drag=null;syncControls();storageSave();state.preview=null;scheduleRender(0);
  });
  $('#clearLibrary').onclick=clearLibrary;
  $('#compareBtn').onclick=()=>void compare();
  $('#closeCompare').onclick=()=>$('#compareDialog').close();
  $('#downloadConfig').onclick=downloadSettings;
  $('#importConfig').addEventListener('change',async e=>{
    const file=e.target.files?.[0];if(!file)return;
    try{const obj=JSON.parse(await file.text());state.settings=validateSettings(obj.settings||obj);
      state.preset='custom';syncControls();storageSave();state.preview=null;scheduleRender(0);message('Réglages importés');}
    catch(err){message('Réglages invalides : '+err.message);}e.target.value='';
  });
  document.addEventListener('keydown',e=>{
    if(e.key==='Escape'&&$('#compareDialog').open)$('#compareDialog').close();
    if(e.target.matches('input,select,textarea')||e.ctrlKey||e.metaKey)return;
    const i=state.files.findIndex(f=>f.id===state.selected?.id);
    if(e.key==='ArrowDown'&&i<state.files.length-1){selectFile(state.files[i+1]);e.preventDefault();}
    if(e.key==='ArrowUp'&&i>0){selectFile(state.files[i-1]);e.preventDefault();}
  });
}
function switchPage(page){
  const studio=page==='studio';const frame=page==='frame';const firmware=page==='firmware';
  $('#workspace').hidden=!studio;$('#framePage').hidden=!frame;$('#firmwarePage').hidden=!firmware;
  $('#tabStudio').classList.toggle('selected',studio);
  $('#tabFrame').classList.toggle('selected',frame);
  $('#tabFirmware').classList.toggle('selected',firmware);
  $('.top-actions').hidden=!studio;
}
storageLoad();createPalette();syncControls();wireEvents();renderFileList();startFrameUI();startFirmwareUI();updateExportLabels();

if('serviceWorker' in navigator && location.protocol!=='file:')navigator.serviceWorker.register('./sw.js').catch(()=>{});
