// PhotoPainter native PPEF/EPD v1. Exactly 24-byte header + 192000 packed pixels.
import {WIDTH, HEIGHT, PALETTE} from './engine.js';
const BYTE_COUNT = WIDTH * HEIGHT / 2;
const expectedRGB = new Map(PALETTE.map((c,i)=>[c.join(','),i]));
const crcTable = new Uint32Array(256);
for(let i=0;i<256;i++){let c=i;for(let j=0;j<8;j++)c=c&1?(c>>>1)^0xEDB88320:c>>>1;crcTable[i]=c>>>0;}
export function crc32(bytes){let crc=0xFFFFFFFF;for(const b of bytes)crc=crcTable[(crc^b)&255]^(crc>>>8);return (crc^0xFFFFFFFF)>>>0;}
export function encodeEPD(image,orientation=0){
  const {width,height,data}=image,angle=Number(orientation);
  if(![0,90,180,270].includes(angle))throw Error('Orientation invalide');
  const portrait=angle===90||angle===270;
  if(width!==(portrait?HEIGHT:WIDTH)||height!==(portrait?WIDTH:HEIGHT))throw Error('Dimensions incompatibles avec orientation');
  if(data.length!==width*height*4)throw Error('Pixels RGBA incomplets');
  const bytes=new Uint8Array(24+BYTE_COUNT);const view=new DataView(bytes.buffer);
  bytes.set([80,80,69,70],0);view.setUint16(4,1,true);
  view.setUint16(6,WIDTH,true);view.setUint16(8,HEIGHT,true);
  bytes[10]=1;bytes[11]=0;view.setUint32(12,BYTE_COUNT,true);
  // Logical (portrait/landscape) coordinates are mapped to physical panel order.
  // Untouched output bytes are initialized to WHITE (index 1), though all are filled.
  bytes.fill(0x11,24);
  for(let y=0;y<height;y++)for(let x=0;x<width;x++){
    const pos=(y*width+x)*4;
    const key=`${data[pos]},${data[pos+1]},${data[pos+2]}`;
    const color=expectedRGB.get(key);
    if(color===undefined)throw Error('Image non quantifiée sur la palette 7 couleurs');
    let px=x,py=y;
    if(angle===90){px=WIDTH-1-y;py=x;}
    else if(angle===180){px=WIDTH-1-x;py=HEIGHT-1-y;}
    else if(angle===270){px=y;py=HEIGHT-1-x;}
    const offset=24+py*(WIDTH/2)+(px>>1);
    if(px&1)bytes[offset]=(bytes[offset]&0xF0)|color;
    else bytes[offset]=(bytes[offset]&0x0F)|(color<<4);
  }
  view.setUint32(16,crc32(bytes.subarray(24)),true);
  view.setUint32(20,0,true);
  return bytes;
}
export function decodeEPD(bytes){
  const arr=bytes instanceof Uint8Array?bytes:new Uint8Array(bytes);
  if(arr.length!==24+BYTE_COUNT)throw Error('EPD incorrect: longueur');
  const view=new DataView(arr.buffer,arr.byteOffset,arr.byteLength);
  if(String.fromCharCode(...arr.subarray(0,4))!=='PPEF'||view.getUint16(4,true)!==1||view.getUint16(6,true)!==WIDTH||view.getUint16(8,true)!==HEIGHT||arr[10]!==1||arr[11]!==0||view.getUint32(12,true)!==BYTE_COUNT||view.getUint32(20,true)!==0)throw Error('EPD incorrect: en-tête');
  if(view.getUint32(16,true)!==crc32(arr.subarray(24)))throw Error('EPD incorrect: CRC');
  for(const b of arr.subarray(24))if((b&15)>6||(b>>>4)>6)throw Error('EPD incorrect: palette');
  return arr.subarray(24);
}
