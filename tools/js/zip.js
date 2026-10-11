/* Minimal lossless ZIP archive creator (store method, no external CDN dependency). */
const crcTable=new Uint32Array(256);
for(let n=0;n<256;n++){let c=n;for(let k=0;k<8;k++)c=c&1?0xedb88320^(c>>>1):c>>>1;crcTable[n]=c>>>0;}
function crc32(bytes){let c=0xffffffff;for(const b of bytes)c=crcTable[(c^b)&255]^(c>>>8);return(c^0xffffffff)>>>0;}
const enc=new TextEncoder();
const u16=(v,o,n)=>v.setUint16(o,n,true),u32=(v,o,n)=>v.setUint32(o,n>>>0,true);
export function makeZip(items){
  if(!items.length)throw Error('Archive vide');
  const local=[],central=[];let offset=0;
  const now=new Date(),date=((Math.max(1980,now.getFullYear())-1980)<<9)|((now.getMonth()+1)<<5)|now.getDate();
  const time=(now.getHours()<<11)|(now.getMinutes()<<5)|(now.getSeconds()>>1);
  for(const item of items){
    const name=enc.encode(item.name),payload=item.data instanceof Uint8Array?item.data:new Uint8Array(item.data);
    if(payload.byteLength>0xffffffff)throw Error('Fichier trop grand');
    const crc=crc32(payload);const loc=new Uint8Array(30+name.length),lv=new DataView(loc.buffer);
    u32(lv,0,0x04034b50);u16(lv,4,20);u16(lv,6,0x800);u16(lv,8,0);
    u16(lv,10,time);u16(lv,12,date);u32(lv,14,crc);u32(lv,18,payload.length);u32(lv,22,payload.length);
    u16(lv,26,name.length);loc.set(name,30);
    local.push(loc,payload);
    const cen=new Uint8Array(46+name.length),cv=new DataView(cen.buffer);
    u32(cv,0,0x02014b50);u16(cv,4,20);u16(cv,6,20);u16(cv,8,0x800);
    u16(cv,10,0);u16(cv,12,time);u16(cv,14,date);u32(cv,16,crc);
    u32(cv,20,payload.length);u32(cv,24,payload.length);u16(cv,28,name.length);
    u32(cv,42,offset);cen.set(name,46);
    central.push(cen);offset+=loc.length+payload.length;
  }
  const centralSize=central.reduce((s,x)=>s+x.length,0),footer=new Uint8Array(22),fv=new DataView(footer.buffer);
  u32(fv,0,0x06054b50);u16(fv,8,items.length);u16(fv,10,items.length);
  u32(fv,12,centralSize);u32(fv,16,offset);
  return new Blob([...local,...central,footer],{type:'application/zip'});
}
export function saveFile(data,name,type){
  const blob=data instanceof Blob?data:new Blob([data],{type:type||'application/octet-stream'});
  const url=URL.createObjectURL(blob),a=document.createElement('a');
  a.href=url;a.download=name;document.body.append(a);a.click();a.remove();
  setTimeout(()=>URL.revokeObjectURL(url),2000);
}
