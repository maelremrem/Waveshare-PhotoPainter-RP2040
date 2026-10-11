import assert from 'node:assert/strict';
import {PALETTE,WIDTH,HEIGHT} from '../js/engine.js';
import {encodeEPD,decodeEPD,crc32} from '../js/epd.js';
const angles=[0,90,180,270];
for(const orientation of angles){
  const portrait=orientation===90||orientation===270;
  const width=portrait?HEIGHT:WIDTH,height=portrait?WIDTH:HEIGHT;
  const rgba=new Uint8ClampedArray(width*height*4);
  for(let y=0;y<height;y++)for(let x=0;x<width;x++){
    const index=(x*7/width|0)%7,p=PALETTE[index],i=(y*width+x)*4;
    rgba.set([p[0],p[1],p[2],255],i);
  }
  const encoded=encodeEPD({width,height,data:rgba},orientation);
  assert.equal(encoded.length,192024);
  assert.equal(Buffer.from(encoded.slice(0,4)).toString('ascii'),'PPEF');
  assert.equal(encoded[10],1);
  assert.equal(new DataView(encoded.buffer).getUint32(12,true),192000);
  const pix=decodeEPD(encoded);assert.equal(pix.length,192000);
  assert.equal(crc32(pix),new DataView(encoded.buffer).getUint32(16,true));
  // Pixel coordinates: logical -> physical rotation, left nibble is even x.
  for(const [x,y] of [[0,0],[1,1],[width>>1,height>>1],[width-1,height-1]]){
    let px=x,py=y;
    if(orientation===90){px=WIDTH-1-y;py=x;}
    if(orientation===180){px=WIDTH-1-x;py=HEIGHT-1-y;}
    if(orientation===270){px=y;py=HEIGHT-1-x;}
    const expected=(x*7/width|0)%7,b=encoded[24+py*(WIDTH/2)+(px>>1)];
    assert.equal(px%2?b&15:b>>4,expected,`rotation ${orientation}, ${x},${y}`);
  }
  const tampered=encoded.slice();tampered[27]^=1;
  assert.throws(()=>decodeEPD(tampered),/CRC/);
  console.log('EPD verified',orientation);
}
// Native exports must be fully quantized: reject arbitrary 8-bit RGB.
const bad=new Uint8ClampedArray(WIDTH*HEIGHT*4);bad.fill(123);
assert.throws(()=>encodeEPD({width:WIDTH,height:HEIGHT,data:bad}),/palette/);
