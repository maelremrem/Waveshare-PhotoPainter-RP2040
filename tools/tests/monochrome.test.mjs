import assert from 'node:assert/strict';
import {DEFAULTS, PRESETS, ALGORITHMS, MONO_MODES, PALETTE, paletteFor, quantize, bmp24} from '../js/engine.js';

const w=24,h=17,rgba=new Uint8ClampedArray(w*h*4);
for(let y=0;y<h;y++)for(let x=0;x<w;x++){
  const p=(y*w+x)*4;
  rgba[p]=Math.round(x*255/(w-1));
  rgba[p+1]=Math.round(y*255/(h-1));
  rgba[p+2]=Math.round(((x+y)/(w+h-2))*255);
  rgba[p+3]=255;
}
const source={width:w,height:h,data:rgba};
const allowed=new Set(['0,0,0','255,255,255']);
for(const mode of Object.keys(MONO_MODES).filter(m=>m!=='off')){
  assert.deepEqual(paletteFor({...DEFAULTS,monoMode:mode}),PALETTE.slice(0,2));
  for(const algorithm of Object.keys(ALGORITHMS)){
    const config={...DEFAULTS,monoMode:mode,algorithm};
    const image=quantize(source,config);
    const colors=new Set();
    for(let i=0;i<image.data.length;i+=4) {
      const col=Array.from(image.data.slice(i,i+3)).join(',');
      assert(allowed.has(col),`${mode} / ${algorithm}: ${col}`);
      assert.equal(image.data[i+3],255);
      colors.add(col);
    }
    assert(colors.size>0);
    const bmp=bmp24(image);
    assert.equal(new DataView(bmp.buffer).getUint16(28,true),24);
  }
  console.log('PASS',mode,'all 11 dithering algorithms');
}
const bwPresetKeys=['bw-photo','bw-soft','bw-contrast','bw-ink'];
for(const key of bwPresetKeys){assert(PRESETS[key]?.monoMode!=='off');}
for(const key of ['natural','soft','vivid','illustration','ordered']){assert.equal(PRESETS[key].monoMode,'off');}
const plain={width:2,height:1,data:Uint8ClampedArray.from([0,0,0,255,255,255,255,255])};
const normal=quantize(plain,{...DEFAULTS,monoMode:'threshold',monoThreshold:128,algorithm:'floyd'});
assert.deepEqual(Array.from(normal.data),[0,0,0,255,255,255,255,255]);
const inverted=quantize(plain,{...DEFAULTS,monoMode:'threshold',monoThreshold:128,monoInvert:true});
assert.deepEqual(Array.from(inverted.data),[255,255,255,255,0,0,0,255]);
assert.deepEqual(Array.from(quantize(plain,{...DEFAULTS,monoMode:'threshold',algorithm:'floyd'}).data),
  Array.from(quantize(plain,{...DEFAULTS,monoMode:'threshold',algorithm:'bayer8'}).data),
  'threshold must ignore dithering');
assert.equal(paletteFor(DEFAULTS).length,7);
console.log('Monochrome tests OK');
