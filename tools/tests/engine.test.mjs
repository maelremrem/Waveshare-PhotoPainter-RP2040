import assert from 'node:assert/strict';
import {PALETTE,DEFAULTS,PRESETS,quantize,bmp24,outputSize,sanitizeBaseName,ALGORITHMS} from '../js/engine.js';
import {makeZip} from '../js/zip.js';

const width=32,height=20,source=new Uint8ClampedArray(width*height*4);
for(let y=0;y<height;y++)for(let x=0;x<width;x++){
  const i=(y*width+x)*4;source[i]=x*255/(width-1);source[i+1]=y*255/(height-1);
  source[i+2]=(x+y)*255/(width+height-2);source[i+3]=255;
}
const image={width,height,data:source};
assert.deepEqual(outputSize(0),{width:800,height:480});
assert.deepEqual(outputSize(90),{width:480,height:800});
assert.equal(sanitizeBaseName('été soleil.jpg'),'ete_soleil');
for(const algorithm of Object.keys(ALGORITHMS)) {
  const result=quantize(image,{...DEFAULTS,algorithm});
  assert.equal(result.data.length,width*height*4,algorithm);
  const allowed=new Set(PALETTE.map(p=>p.join(',')));
  for(let i=0;i<result.data.length;i+=4){
    assert.equal(result.data[i+3],255,algorithm);
    assert(allowed.has(Array.from(result.data.slice(i,i+3)).join(',')),algorithm+' palette mismatch');
  }
  const bmp=bmp24(result);
  const view=new DataView(bmp.buffer);
  assert.equal(bmp[0],66);assert.equal(bmp[1],77);
  assert.equal(view.getInt32(18,true),width);assert.equal(view.getInt32(22,true),height);
  assert.equal(view.getUint16(28,true),24);
  const zip=makeZip([{name:'pic/photo.bmp',data:bmp}]);
  assert(zip.size>=bmp.length);
  console.log('PASS',algorithm);
}
console.log('Engine tests OK');
